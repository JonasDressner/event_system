#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#endif

class IPCWriter {
public:
#ifdef _WIN32
    IPCWriter(const std::string& pipeName) : pipeName(pipeName), pipe(INVALID_HANDLE_VALUE) {
        connect();
    }

    ~IPCWriter() {
        if (pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe);
        }
    }

    void write(const std::string& message) {
        if (pipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Pipe not connected");
        }
        
        DWORD bytesWritten = 0;
        if (!WriteFile(pipe, message.c_str(), message.length(), &bytesWritten, NULL)) {
            throw std::runtime_error("Failed to write to pipe");
        }
    }

private:
    std::string pipeName;
    HANDLE pipe;

    void connect() {
        pipe = CreateFileA(
            ("\\\\.\\pipe\\" + pipeName).c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (pipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open pipe for writing");
        }
    }

#else // Linux
    IPCWriter(const std::string& pipeName) : pipeName("/tmp/" + pipeName), fd(-1) {
        connect();
    }

    ~IPCWriter() {
        if (fd != -1) {
            close(fd);
        }
    }

    void write(const std::string& message) {
        if (fd == -1) {
            throw std::runtime_error("Pipe not connected");
        }
        
        if (::write(fd, message.c_str(), message.length()) < 0) {
            throw std::runtime_error("Failed to write to pipe");
        }
    }

private:
    std::string pipeName;
    int fd;

    void connect() {
        // Create FIFO if it doesn't exist
        mkfifo(pipeName.c_str(), 0666);
        
        fd = open(pipeName.c_str(), O_WRONLY);
        if (fd == -1) {
            throw std::runtime_error("Failed to open pipe for writing");
        }
    }
#endif
};

class IPCReader {
public:
#ifdef _WIN32
    IPCReader(const std::string& pipeName) : pipeName(pipeName), pipe(INVALID_HANDLE_VALUE) {
        createPipe();
    }

    ~IPCReader() {
        if (pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe);
        }
    }

    bool read(std::string& message, int timeoutMs = 100) {
        if (pipe == INVALID_HANDLE_VALUE) {
            return false;
        }

        char buffer[4096] = {0};
        DWORD bytesRead = 0;
        
        if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            DWORD err = GetLastError();
            if (err == ERROR_PIPE_LISTENING || err == ERROR_NO_DATA) {
                return false;
            }
            return false;
        }

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            message = buffer;
            return true;
        }
        return false;
    }

private:
    std::string pipeName;
    HANDLE pipe;

    void createPipe() {
        pipe = CreateNamedPipeA(
            ("\\\\.\\pipe\\" + pipeName).c_str(),
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            NULL
        );
        if (pipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to create named pipe");
        }
    }

#else // Linux
    IPCReader(const std::string& pipeName) : pipeName("/tmp/" + pipeName), fd(-1) {
        createPipe();
    }

    ~IPCReader() {
        if (fd != -1) {
            close(fd);
        }
        unlink(pipeName.c_str());
    }

    bool read(std::string& message, int timeoutMs = 100) {
        if (fd == -1) {
            return false;
        }

        char buffer[4096] = {0};
        ssize_t bytesRead = ::read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            message = buffer;
            return true;
        }
        return false;
    }

private:
    std::string pipeName;
    int fd;

    void createPipe() {
        // Remove old FIFO if it exists
        unlink(pipeName.c_str());
        
        // Create FIFO
        if (mkfifo(pipeName.c_str(), 0666) == -1) {
            throw std::runtime_error("Failed to create FIFO");
        }
        
        fd = open(pipeName.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            throw std::runtime_error("Failed to open FIFO for reading");
        }
    }
#endif
};
