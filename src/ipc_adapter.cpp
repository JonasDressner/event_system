#include "ipc_adapter.hpp"

IPCWriterAdapter::IPCWriterAdapter(const std::string& pipeName)
    : writer(pipeName) {}

void IPCWriterAdapter::write(const std::string& msg) {
    writer.write(msg);
}

IPCReaderAdapter::IPCReaderAdapter(const std::string& pipeName)
    : reader(pipeName) {}

bool IPCReaderAdapter::read(std::string& msg, int timeoutMs) {
    return reader.read(msg, timeoutMs);
}