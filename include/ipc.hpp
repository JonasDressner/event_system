#pragma once

#include "iipc.hpp"
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#endif

class IPCWriter : public IIPCWriter {
public:
    explicit IPCWriter(const std::string& pipeName);
    void write(const std::string& message) override;
    ~IPCWriter() override;

private:
    std::string pipeName_;
#ifdef _WIN32
    HANDLE pipe_;
#else
    int fd_;
#endif

    void connect();
};

class IPCReader : public IIPCReader {
public:
    explicit IPCReader(const std::string& pipeName);
    bool read(std::string& message, int timeoutMs = 100) override;
    ~IPCReader() override;

private:
    std::string pipeName_;
#ifdef _WIN32
    HANDLE pipe_;
#else
    int fd_;
#endif

    void createPipe();
};
