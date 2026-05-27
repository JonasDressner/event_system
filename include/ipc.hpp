#pragma once

#include "iipc.hpp"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// Producer side: creates the named pipe / FIFO and waits for the consumer.
class IPCWriter : public IIPCWriter {
public:
    explicit IPCWriter(const std::string& pipeName);
    void write(const std::string& message) override;
    ~IPCWriter() override;

private:
    std::string pipeName_;
#ifdef _WIN32
    HANDLE pipe_{INVALID_HANDLE_VALUE};
#else
    int fd_{-1};
#endif

    void createPipe(); // server role: create + wait for first client
    void reconnect();  // called internally when consumer disconnects
};

// Consumer side: connects to the pipe created by the producer.
class IPCReader : public IIPCReader {
public:
    explicit IPCReader(const std::string& pipeName);
    bool read(std::string& message, int timeoutMs = 100) override;
    ~IPCReader() override;

private:
    std::string pipeName_;
#ifdef _WIN32
    HANDLE pipe_{INVALID_HANDLE_VALUE};
#else
    int fd_{-1};
#endif

    void connect(); // client role: retry-connect to producer's pipe
};
