#pragma once

#include "iipc_factory.hpp"

class PipeIPCFactory : public IIPCFactory {
public:
    std::unique_ptr<IIPCWriter> createWriter(const std::string& name) override;
    std::unique_ptr<IIPCReader> createReader(const std::string& name) override;
};
