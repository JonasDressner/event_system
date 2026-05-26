#include "ipc_factory.hpp"
#include "ipc.hpp"

std::unique_ptr<IIPCWriter> createIPCWriter(const std::string& pipeName) {
    return std::make_unique<IPCWriter>(pipeName);
}

std::unique_ptr<IIPCReader> createIPCReader(const std::string& pipeName) {
    return std::make_unique<IPCReader>(pipeName);
}