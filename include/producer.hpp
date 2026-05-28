#pragma once

#include "event.hpp"
#include "ievent_serializer.hpp"
#include "iipc_factory.hpp"
#include "signal_helper.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <string_view>

class Producer : public IStopper {
public:
    Producer(std::unique_ptr<IIPCWriter> writer, std::shared_ptr<IEventSerializer> serializer);

    Producer(const std::string& pipeName, IIPCFactory& factory);

    Event generateEvent();
    void start();
    void stop() noexcept override;

private:
    // Compile-time tables — sizes drive the distributions automatically.
    // To add a component or message, only extend the array; nothing else changes.
    static constexpr std::array<std::string_view, 5> cComponents = {"Database",
                                                                    "API",
                                                                    "Cache",
                                                                    "Auth",
                                                                    "Logging"};
    static constexpr std::array<Severity, 3> cSeverities = {Severity::INFO,
                                                            Severity::WARNING,
                                                            Severity::ERROR};
    static constexpr std::array<std::string_view, 5> cMessages = {
        "Operation completed successfully",
        "Processing request",
        "Performance degradation detected",
        "Connection timeout",
        "Critical system error",
    };

    std::unique_ptr<IIPCWriter> writer_;
    std::shared_ptr<IEventSerializer> serializer_;
    std::atomic<bool> running_;

    std::mt19937 gen_;
    std::uniform_int_distribution<> severityDist_;
    std::uniform_int_distribution<> componentDist_;
    std::uniform_int_distribution<> messageDist_;
};
