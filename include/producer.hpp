#pragma once

#include "event.hpp"
#include "ievent_serializer.hpp"
#include "iipc_factory.hpp"
#include "signal_helper.hpp"

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <vector>

class Producer : public IStopper {
public:
    Producer(std::unique_ptr<IIPCWriter> writer, std::shared_ptr<IEventSerializer> serializer);

    Producer(const std::string& pipeName, IIPCFactory& factory);

    Event generateEvent();
    void start();
    void stop() noexcept override;

private:
    std::unique_ptr<IIPCWriter> writer_;
    std::shared_ptr<IEventSerializer> serializer_;
    std::atomic<bool> running_;

    std::vector<std::string> components_;
    std::mt19937 gen_;
    std::uniform_int_distribution<> severityDist_;
    std::uniform_int_distribution<> componentDist_;
    std::uniform_int_distribution<> messageDist_;
};
