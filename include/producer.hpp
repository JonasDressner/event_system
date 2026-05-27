#pragma once

#include "event.hpp"
#include "iipc.hpp"
#include "iipc_factory.hpp"
#include "ievent_serializer.hpp"
#include "signal_helper.hpp"
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <atomic>

class Producer : public IStopper {
public:
    Producer(std::unique_ptr<IIPCWriter> writer,
             std::shared_ptr<IEventSerializer> serializer);

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
    std::uniform_int_distribution<> severity_dist_;
    std::uniform_int_distribution<> component_dist_;
    std::uniform_int_distribution<> message_dist_;
};
