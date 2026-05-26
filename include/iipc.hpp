#pragma once
#include <string>

struct IIPCWriter {
    virtual ~IIPCWriter() = default;
    virtual void write(const std::string& msg) = 0;
};

struct IIPCReader {
    virtual ~IIPCReader() = default;
    virtual bool read(std::string& msg, int timeoutMs = 100) = 0;
};