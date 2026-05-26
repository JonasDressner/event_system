#pragma once

#include "event.hpp"
#include "iipc.hpp"
#include "ievent_serializer.hpp"
#include "event_serializer.hpp"
#include "ipc_factory.hpp"
#include "signal_helper.hpp"
#include <random>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

class Producer : public IStopper {
public:
    Producer(std::unique_ptr<IIPCWriter> writer,
             std::shared_ptr<IEventSerializer> serializer)
        : writer_(std::move(writer))
        , serializer_(std::move(serializer))
        , running_(true)
    {
        components_ = {"Database", "API", "Cache", "Auth", "Logging"};
        severity_dist_ = std::uniform_int_distribution<>(0, 2);
    }

    explicit Producer(const std::string& pipeName)
      : Producer(createIPCWriter(pipeName), std::make_shared<SimpleEventSerializer>())
    {}

    Event generateEvent() {
        Event evt;
        evt.timestamp = std::chrono::system_clock::now();
        evt.component = components_[component_dist_(gen_)];
        evt.severity = static_cast<Severity>(severity_dist_(gen_));

        const std::string messages[] = {
            "Operation completed successfully",
            "Processing request",
            "Performance degradation detected",
            "Connection timeout",
            "Critical system error"
        };
        evt.message = messages[message_dist_(gen_)];
        return evt;
    }

    void start() {
        std::cout << "[Producer] Starting event generation..." << std::endl;
        int eventCount = 0;

        while (running_.load()) {
            try {
                Event evt = generateEvent();
                std::string serialized = serializer_->serialize(evt);
                writer_->write(serialized);

                std::cout << "[Producer] Event #" << (++eventCount)
                          << " sent: " << evt.component
                          << " [" << severityToString(evt.severity) << "]"
                          << std::endl;

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } catch (const std::exception& e) {
                std::cerr << "[Producer] Error: " << e.what() << std::endl;
                break;
            }
        }
    }

    void stop() noexcept override {
        running_.store(false);
    }

private:
    std::unique_ptr<IIPCWriter> writer_;
    std::shared_ptr<IEventSerializer> serializer_;
    std::atomic<bool> running_;

    std::vector<std::string> components_;
    std::mt19937 gen_{std::random_device{}()};
    std::uniform_int_distribution<> severity_dist_;
    std::uniform_int_distribution<> component_dist_{0, 4};
    std::uniform_int_distribution<> message_dist_{0, 4};
};
