#pragma once

#include "event.hpp"

#include <string>

struct IEventSerializer {
    virtual ~IEventSerializer() = default;
    virtual std::string serialize(const Event& e) = 0;
    virtual Event deserialize(const std::string& s) = 0;
};