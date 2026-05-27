#include "ipc_factory.hpp"

#include "ipc.hpp"

std::unique_ptr<IIPCWriter> PipeIPCFactory::createWriter(const std::string& name) {
    return std::make_unique<IPCWriter>(name);
}

std::unique_ptr<IIPCReader> PipeIPCFactory::createReader(const std::string& name) {
    return std::make_unique<IPCReader>(name);
}
