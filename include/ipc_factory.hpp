#pragma once

#include "iipc.hpp"
#include <memory>
#include <string>

std::unique_ptr<IIPCWriter> createIPCWriter(const std::string& pipeName);
std::unique_ptr<IIPCReader> createIPCReader(const std::string& pipeName);