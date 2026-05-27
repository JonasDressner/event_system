#include "ipc.hpp"

#include <stdexcept>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <sstream>

static std::string getLastWin32Error() {
    DWORD err = GetLastError();
    char buf[256];
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, static_cast<DWORD>(sizeof(buf)), nullptr
    );
    std::string msg = size ? std::string(buf, size) : "Unknown error";
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
        msg.pop_back();
    return msg + " (" + std::to_string(err) + ")";
}
#else
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#endif

// ============================================================
//  IPCWriter  (Producer = pipe server)
// ============================================================

IPCWriter::IPCWriter(const std::string& pipeName)
    : pipeName_(pipeName)
{
    createPipe();
}

IPCWriter::~IPCWriter() {
#ifdef _WIN32
    if (pipe_ != INVALID_HANDLE_VALUE)
        CloseHandle(pipe_);
#else
    if (fd_ != -1)
        close(fd_);
    unlink(pipeName_.c_str());
#endif
}

void IPCWriter::write(const std::string& message) {
#ifdef _WIN32
    if (pipe_ == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Pipe not connected");

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe_, message.c_str(), static_cast<DWORD>(message.length()), &bytesWritten, nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
            reconnect();
            if (!WriteFile(pipe_, message.c_str(), static_cast<DWORD>(message.length()), &bytesWritten, nullptr))
                throw std::runtime_error("Failed to write after reconnect: " + getLastWin32Error());
            return;
        }
        throw std::runtime_error("Failed to write to pipe: " + getLastWin32Error());
    }
#else
    if (fd_ == -1)
        throw std::runtime_error("Pipe not connected");

    ssize_t result = ::write(fd_, message.c_str(), message.length());
    if (result < 0) {
        if (errno == EPIPE) {
            reconnect();
            result = ::write(fd_, message.c_str(), message.length());
            if (result < 0)
                throw std::runtime_error(std::string("Failed to write after reconnect: ") + std::strerror(errno));
            return;
        }
        throw std::runtime_error(std::string("Failed to write to pipe: ") + std::strerror(errno));
    }
#endif
}

void IPCWriter::createPipe() {
#ifdef _WIN32
    std::string fullPipeName = "\\\\.\\pipe\\" + pipeName_;
    pipe_ = CreateNamedPipeA(
        fullPipeName.c_str(),
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 4096, 4096, 0, nullptr
    );
    if (pipe_ == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Failed to create named pipe: " + getLastWin32Error());

    std::cout << "[IPC] Pipe created. Waiting for consumer to connect..." << std::endl;
    if (!ConnectNamedPipe(pipe_, nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            CloseHandle(pipe_);
            pipe_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Failed to connect named pipe: " + getLastWin32Error());
        }
    }
    std::cout << "[IPC] Consumer connected." << std::endl;
#else
    if (mkfifo(pipeName_.c_str(), 0666) == -1 && errno != EEXIST)
        throw std::runtime_error(std::string("Failed to create FIFO: ") + std::strerror(errno));

    std::cout << "[IPC] FIFO created. Waiting for consumer to connect..." << std::endl;
    fd_ = open(pipeName_.c_str(), O_WRONLY);
    if (fd_ == -1)
        throw std::runtime_error(std::string("Failed to open pipe for writing: ") + std::strerror(errno));

    std::cout << "[IPC] Consumer connected." << std::endl;
#endif
}

void IPCWriter::reconnect() {
#ifdef _WIN32
    DisconnectNamedPipe(pipe_);
    std::cout << "[IPC] Consumer disconnected. Waiting for new consumer to connect..." << std::endl;
    if (!ConnectNamedPipe(pipe_, nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            CloseHandle(pipe_);
            pipe_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Failed to reconnect named pipe: " + getLastWin32Error());
        }
    }
    std::cout << "[IPC] New consumer connected." << std::endl;
#else
    close(fd_);
    fd_ = -1;
    std::cout << "[IPC] Consumer disconnected. Waiting for new consumer to connect..." << std::endl;
    fd_ = open(pipeName_.c_str(), O_WRONLY);
    if (fd_ == -1)
        throw std::runtime_error(std::string("Failed to reopen pipe for writing: ") + std::strerror(errno));
    std::cout << "[IPC] New consumer connected." << std::endl;
#endif
}

// ============================================================
//  IPCReader  (Consumer = pipe client)
// ============================================================

IPCReader::IPCReader(const std::string& pipeName)
    : pipeName_(pipeName)
{
    connect();
}

IPCReader::~IPCReader() {
#ifdef _WIN32
    if (pipe_ != INVALID_HANDLE_VALUE)
        CloseHandle(pipe_);
#else
    if (fd_ != -1)
        close(fd_);
    // No unlink — Producer owns the FIFO lifecycle
#endif
}

bool IPCReader::read(std::string& message, int timeoutMs) {
#ifdef _WIN32
    if (pipe_ == INVALID_HANDLE_VALUE)
        return false;

    // Poll with PeekNamedPipe until data arrives or timeout expires
    ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(timeoutMs < 0 ? 0 : timeoutMs);
    while (true) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe_, nullptr, 0, nullptr, &available, nullptr)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE)
                throw PipeDisconnectedException();
            throw std::runtime_error("PeekNamedPipe failed: " + getLastWin32Error());
        }
        if (available > 0)
            break;
        if (GetTickCount64() >= deadline)
            return false;
        Sleep(10);
    }

    char buffer[4096] = {0};
    DWORD bytesRead = 0;
    if (!ReadFile(pipe_, buffer, sizeof(buffer) - 1, &bytesRead, nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_LISTENING || err == ERROR_NO_DATA)
            return false;
        if (err == ERROR_BROKEN_PIPE)
            throw PipeDisconnectedException();
        throw std::runtime_error("Failed to read from pipe: " + getLastWin32Error());
    }
    if (bytesRead > 0) {
        message.assign(buffer, bytesRead);
        return true;
    }
    return false;
#else
    if (fd_ == -1)
        return false;

    // Use poll() to respect the timeout
    struct pollfd pfd;
    pfd.fd      = fd_;
    pfd.events  = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, timeoutMs);
    if (ret < 0)
        throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
    if (ret == 0)
        return false;   // timeout

    char buffer[4096] = {0};
    ssize_t bytesRead = ::read(fd_, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false;
        throw std::runtime_error(std::string("Failed to read from pipe: ") + std::strerror(errno));
    }
    if (bytesRead == 0)
        throw PipeDisconnectedException();

    message.assign(buffer, static_cast<size_t>(bytesRead));
    return true;
#endif
}

void IPCReader::connect() {
#ifdef _WIN32
    std::string fullPipeName = "\\\\.\\pipe\\" + pipeName_;
    const int maxRetries = 50;
    const DWORD retryDelayMs = 200;

    std::cout << "[IPC] Connecting to producer pipe..." << std::endl;
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        pipe_ = CreateFileA(
            fullPipeName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr
        );
        if (pipe_ != INVALID_HANDLE_VALUE) {
            DWORD dwMode = PIPE_READMODE_MESSAGE;
            SetNamedPipeHandleState(pipe_, &dwMode, nullptr, nullptr);
            std::cout << "[IPC] Connected to producer." << std::endl;
            return;
        }
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PIPE_BUSY) {
            std::cout << "[IPC] Pipe not available yet. Please start the producer first. "
                      << "Retrying... (" << (attempt + 1) << "/" << maxRetries << ")" << std::endl;
            Sleep(retryDelayMs);
            continue;
        }
        throw std::runtime_error("Failed to open pipe for reading: " + getLastWin32Error());
    }
    throw std::runtime_error("Failed to open pipe for reading: timeout waiting for producer");
#else
    const int maxRetries = 50;
    std::cout << "[IPC] Connecting to producer pipe..." << std::endl;
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        fd_ = open(pipeName_.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd_ != -1) {
            std::cout << "[IPC] Connected to producer." << std::endl;
            return;
        }
        if (errno == ENOENT) {
            std::cout << "[IPC] Pipe not available yet. Please start the producer first. "
                      << "Retrying... (" << (attempt + 1) << "/" << maxRetries << ")" << std::endl;
            usleep(200000);
            continue;
        }
        throw std::runtime_error(std::string("Failed to open pipe for reading: ") + std::strerror(errno));
    }
    throw std::runtime_error("Failed to open pipe for reading: timeout waiting for producer");
#endif
}
