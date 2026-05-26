#include "ipc_factory.hpp"
#include "ipc_adapter.hpp"

std::unique_ptr<IIPCWriter> createIPCWriter(const std::string& pipeName) {
    return std::make_unique<IPCWriterAdapter>(pipeName);
}

std::unique_ptr<IIPCReader> createIPCReader(const std::string& pipeName) {
    return std::make_unique<IPCReaderAdapter>(pipeName);
}