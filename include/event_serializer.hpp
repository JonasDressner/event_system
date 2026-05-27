#pragma once

#include "ievent_serializer.hpp"

// JSON serialization utilities — implemented in event_serializer.cpp
namespace EventSerializerUtils {
std::string toJson(const Event& e);
Event fromJson(const std::string& json);
} // namespace EventSerializerUtils

class EventSerializer : public IEventSerializer {
public:
    std::string serialize(const Event& e) override;
    Event deserialize(const std::string& s) override;
};
