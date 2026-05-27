#pragma once
#include "event.hpp"
#include "event_serializer.hpp"
#include "ievent_serializer.hpp"
#include "iipc.hpp"

#include <string>
#include <vector>

struct FakeWriter : IIPCWriter {
    std::vector<std::string> sent;

    void write(const std::string& msg) override { sent.push_back(msg); }
};

struct FakeReader : IIPCReader {
    std::vector<std::string> toSend;

    bool read(std::string& msg, int) override {
        if (toSend.empty())
            return false;
        msg = toSend.front();
        toSend.erase(toSend.begin());
        return true;
    }
};

struct FakeSerializer : IEventSerializer {
    std::string serialize(const Event& e) override { return EventSerializerUtils::toJson(e); }

    Event deserialize(const std::string& s) override { return EventSerializerUtils::fromJson(s); }
};