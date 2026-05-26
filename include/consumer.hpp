#pragma once

#include "event.hpp"
#include "iipc.hpp"
#include "ievent_serializer.hpp"
#include "event_serializer.hpp"
#include "ipc_factory.hpp"
#include "signal_helper.hpp"
#include <map>
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <mutex>

struct Statistics {
    int warning_count = 0;
    int error_count = 0;
    std::map<std::string, std::map<std::string, int>> component_severity_count;
};

class Consumer : public IStopper {
public:
    Consumer(std::unique_ptr<IIPCReader> reader,
             std::shared_ptr<IEventSerializer> serializer)
        : reader_(std::move(reader))
        , serializer_(std::move(serializer))
        , running_(true)
        , lastStatsTime_(std::chrono::steady_clock::now())
    {}

    explicit Consumer(const std::string& pipeName)
      : Consumer(createIPCReader(pipeName), std::make_shared<SimpleEventSerializer>())
    {}

    void start() {
        std::cout << "[Consumer] Starting event processing..." << std::endl;

        while (running_.load()) {
            try {
                std::string message;
                if (reader_->read(message, 100)) {
                    Event evt = serializer_->deserialize(message);
                    processEvent(evt);
                }

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime_);
                if (elapsed.count() >= 5) {
                    printStatistics();
                    lastStatsTime_ = now;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (const std::exception& e) {
                std::cerr << "[Consumer] Error: " << e.what() << std::endl;
                break;
            }
        }
    }

    void stop() noexcept override {
        running_.store(false);
    }

    void processEvent(const Event& evt) {
        if (evt.severity < Severity::WARNING) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (evt.severity == Severity::WARNING) {
                stats_.warning_count++;
            } else if (evt.severity == Severity::ERROR_LEVEL) {
                stats_.error_count++;
            }
            stats_.component_severity_count[evt.component][severityToString(evt.severity)]++;
        }

        std::cout << "[Consumer] Event received: " << evt.component << " ["
                  << severityToString(evt.severity) << "] " << evt.message << std::endl;
    }

    Statistics getStatistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_;
    }

private:
    void printStatistics() {
        Statistics statsCopy = getStatistics();

        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "EVENT STATISTICS" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Total WARNING events: " << statsCopy.warning_count << std::endl;
        std::cout << "Total ERROR events:   " << statsCopy.error_count << std::endl;
        std::cout << "\nBreakdown by Component:" << std::endl;
        std::cout << std::string(70, '-') << std::endl;

        for (const auto& [component, severity_map] : statsCopy.component_severity_count) {
            std::cout << "  [" << component << "]" << std::endl;
            for (const auto& [severity, count] : severity_map) {
                std::cout << "    " << std::setw(10) << severity << ": " << count << std::endl;
            }
        }

        std::cout << std::string(70, '=') << "\n" << std::endl;
    }

    std::unique_ptr<IIPCReader> reader_;
    std::shared_ptr<IEventSerializer> serializer_;
    std::atomic<bool> running_;
    Statistics stats_;
    mutable std::mutex stats_mutex_;
    std::chrono::steady_clock::time_point lastStatsTime_;
};
