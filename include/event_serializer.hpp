#pragma once
#include "ievent_serializer.hpp"

class SimpleEventSerializer : public IEventSerializer {
public:
    std::string serialize(const Event& e) override {
        return e.serialize();
    }
    Event deserialize(const std::string& s) override {
        return Event::deserialize(s);
    }
};