#include "ipc.hpp"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <sstream>

static std::string getLastWin32Error() {
    DWORD err = GetLastError();
    char buf[256];
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        static_cast<DWORD>(sizeof(buf)),
        NULL
    );

    std::string msg = size ? std::string(buf, size) : "Unknown error";
    // Trim trailing newline/CRLF from FormatMessageA
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
        msg.pop_back();
    }
    return msg + " (" + std::to_string(err) + ")";
}
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#endif

IPCWriter::IPCWriter(const std::string& pipeName)
    : pipeName_(pipeName)
#ifdef _WIN32
    , pipe_(INVALID_HANDLE_VALUE)
#else
    , fd_(-1)
#endif
{
    connect();
}

IPCWriter::~IPCWriter() {
#ifdef _WIN32
    if (pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
    }
#else
    if (fd_ != -1) {
        close(fd_);
    }
#endif
}

void IPCWriter::write(const std::string& message) {
#ifdef _WIN32
    if (pipe_ == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Pipe not connected");
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe_, message.c_str(), static_cast<DWORD>(message.length()), &bytesWritten, NULL)) {
        throw std::runtime_error("Failed to write to pipe: " + getLastWin32Error());
    }
#else
    if (fd_ == -1) {
        throw std::runtime_error("Pipe not connected");
    }

    ssize_t result = ::write(fd_, message.c_str(), message.length());
    if (result < 0) {
        throw std::runtime_error(std::string("Failed to write to pipe: ") + std::strerror(errno));
    }
#endif
}

void IPCWriter::connect() {
#ifdef _WIN32
    std::string fullPipeName = "\\\\.\\pipe\\" + pipeName_;
    const int maxRetries = 20;
    const DWORD retryDelayMs = 100;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        pipe_ = CreateFileA(
            fullPipeName.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (pipe_ != INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PIPE_BUSY) {
            Sleep(retryDelayMs);
            continue;
        }

        throw std::runtime_error("Failed to open pipe for writing: " + getLastWin32Error());
    }

    throw std::runtime_error("Failed to open pipe for writing: timeout waiting for pipe");
#else
    if (mkfifo(pipeName_.c_str(), 0666) == -1 && errno != EEXIST) {
        throw std::runtime_error(std::string("Failed to create FIFO: ") + std::strerror(errno));
    }

    fd_ = open(pipeName_.c_str(), O_WRONLY);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("Failed to open pipe for writing: ") + std::strerror(errno));
    }
#endif
}

IPCReader::IPCReader(const std::string& pipeName)
    : pipeName_(pipeName)
#ifdef _WIN32
    , pipe_(INVALID_HANDLE_VALUE)
#else
    , fd_(-1)
#endif
{
    createPipe();
}

IPCReader::~IPCReader() {
#ifdef _WIN32
    if (pipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe_);
    }
#else
    if (fd_ != -1) {
        close(fd_);
    }
    unlink(pipeName_.c_str());
#endif
}

bool IPCReader::read(std::string& message, int /*timeoutMs*/) {
#ifdef _WIN32
    if (pipe_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    char buffer[4096] = {0};
    DWORD bytesRead = 0;
    if (!ReadFile(pipe_, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_LISTENING || err == ERROR_NO_DATA || err == ERROR_BROKEN_PIPE) {
            return false;
        }
        throw std::runtime_error("Failed to read from pipe: " + getLastWin32Error());
    }

    if (bytesRead > 0) {
        message.assign(buffer, bytesRead);
        return true;
    }
    return false;
#else
    if (fd_ == -1) {
        return false;
    }

    char buffer[4096] = {0};
    ssize_t bytesRead = ::read(fd_, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        throw std::runtime_error(std::string("Failed to read from pipe: ") + std::strerror(errno));
    }

    if (bytesRead > 0) {
        message.assign(buffer, static_cast<size_t>(bytesRead));
        return true;
    }
    return false;
#endif
}

void IPCReader::createPipe() {
#ifdef _WIN32
    std::string fullPipeName = "\\\\.\\pipe\\" + pipeName_;
    pipe_ = CreateNamedPipeA(
        fullPipeName.c_str(),
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        0,
        NULL
    );

    if (pipe_ == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to create named pipe: " + getLastWin32Error());
    }

    if (!ConnectNamedPipe(pipe_, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            CloseHandle(pipe_);
            pipe_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Failed to connect named pipe: " + getLastWin32Error());
        }
    }
#else
    unlink(pipeName_.c_str());
    if (mkfifo(pipeName_.c_str(), 0666) == -1 && errno != EEXIST) {
        throw std::runtime_error(std::string("Failed to create FIFO: ") + std::strerror(errno));
    }

    fd_ = open(pipeName_.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("Failed to open FIFO for reading: ") + std::strerror(errno));
    }
#endif
}