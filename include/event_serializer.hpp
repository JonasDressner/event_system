#pragma once
#include "ievent_serializer.hpp"
#include "event.hpp"
#include <string>

// Use the standalone EventSerializer utilities (defined in separate header)
namespace EventSerializerUtils {
    std::string toJson(const Event& e);
    Event fromJson(const std::string& json);
}

class SimpleEventSerializer : public IEventSerializer {
public:
    std::string serialize(const Event& e) override {
        return EventSerializerUtils::toJson(e);
    }
    
    Event deserialize(const std::string& s) override {
        return EventSerializerUtils::fromJson(s);
    }
};