#pragma once

#include "iipc.hpp"
#include "ipc.hpp"
#include <string>

class IPCWriterAdapter : public IIPCWriter {
public:
    explicit IPCWriterAdapter(const std::string& pipeName);
    void write(const std::string& msg) override;

private:
    IPCWriter writer;
};

class IPCReaderAdapter : public IIPCReader {
public:
    explicit IPCReaderAdapter(const std::string& pipeName);
    bool read(std::string& msg, int timeoutMs = 100) override;

private:
    IPCReader reader;
};