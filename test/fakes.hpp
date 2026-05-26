#pragma once
#include "iipc.hpp"
#include "ievent_serializer.hpp"
#include "event.hpp"
#include "event_serializer.hpp"
#include <vector>
#include <string>

struct FakeWriter : IIPCWriter {
    std::vector<std::string> sent;
    void write(const std::string& msg) override { sent.push_back(msg); }
};

struct FakeReader : IIPCReader {
    std::vector<std::string> to_send;
    bool read(std::string& msg, int) override {
        if (to_send.empty()) return false;
        msg = to_send.front();
        to_send.erase(to_send.begin());
        return true;
    }
};

struct FakeSerializer : IEventSerializer {
    std::string serialize(const Event& e) override { 
        return EventSerializerUtils::toJson(e); 
    }
    Event deserialize(const std::string& s) override { 
        return EventSerializerUtils::fromJson(s); 
    }
};