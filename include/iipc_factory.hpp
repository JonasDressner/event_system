#pragma once

#include "iipc.hpp"
#include <memory>
#include <string>

struct IIPCFactory {
    virtual ~IIPCFactory() = default;
    virtual std::unique_ptr<IIPCWriter> createWriter(const std::string& name) = 0;
    virtual std::unique_ptr<IIPCReader> createReader(const std::string& name) = 0;
};
