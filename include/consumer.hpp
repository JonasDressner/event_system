#pragma once

#include "event.hpp"
#include "iipc_factory.hpp"
#include "ievent_serializer.hpp"
#include "signal_helper.hpp"
#include <map>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>

struct Statistics {
    int warning_count = 0;
    int error_count = 0;
    std::map<std::string, std::map<std::string, int>> component_severity_count;
};

class Consumer : public IStopper {
public:
    Consumer(std::unique_ptr<IIPCReader> reader,
             std::shared_ptr<IEventSerializer> serializer);

    Consumer(const std::string& pipeName, IIPCFactory& factory);

    void start();
    void stop() noexcept override;
    void processEvent(const Event& evt);
    Statistics getStatistics() const;

private:
    void printStatistics() const;

    std::unique_ptr<IIPCReader> reader_;
    std::shared_ptr<IEventSerializer> serializer_;
    std::atomic<bool> running_;
    Statistics stats_;
    mutable std::mutex stats_mutex_;
    std::chrono::steady_clock::time_point lastStatsTime_;
};
