#pragma once
#include <string>
#include <stdexcept>

// Thrown by IPCReader::read() when the producer has closed the pipe.
class PipeDisconnectedException : public std::runtime_error {
public:
    PipeDisconnectedException()
        : std::runtime_error("Producer has disconnected") {}
};

struct IIPCWriter {
    virtual ~IIPCWriter() = default;
    virtual void write(const std::string& msg) = 0;
};

struct IIPCReader {
    virtual ~IIPCReader() = default;
    virtual bool read(std::string& msg, int timeoutMs = 100) = 0;
};