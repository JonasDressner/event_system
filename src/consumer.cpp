#include "consumer.hpp"

#include "event_serializer.hpp"
#include "iipc_factory.hpp"
#include "signal_helper.hpp"

#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

// ---------- Consumer constructors ----------

Consumer::Consumer(std::unique_ptr<IIPCReader> reader, std::shared_ptr<IEventSerializer> serializer)
    : reader_(std::move(reader)),
      serializer_(std::move(serializer)),
      running_(true),
      lastStatsTime_(std::chrono::steady_clock::now()) {}

Consumer::Consumer(const std::string& pipeName, IIPCFactory& factory)
    : Consumer(factory.createReader(pipeName), std::make_shared<EventSerializer>()) {}

// ---------- Consumer public methods ----------

void Consumer::start() {
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
        } catch (const PipeDisconnectedException&) {
            std::cout << "[Consumer] Producer has disconnected. Shutting down." << std::endl;
            break;
        } catch (const std::exception& e) {
            std::cerr << "[Consumer] Error: " << e.what() << std::endl;
            break;
        }
    }
}

void Consumer::stop() noexcept { running_.store(false); }

void Consumer::processEvent(const Event& evt) {
    if (evt.severity < Severity::WARNING) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        if (evt.severity == Severity::WARNING) {
            stats_.warningCount++;
        } else if (evt.severity == Severity::ERROR) {
            stats_.errorCount++;
        }
        stats_.componentSeverityCount[evt.component][severityToString(evt.severity)]++;
    }

    std::cout << "[" << timestampToString(evt.timestamp) << "]"
              << " [Consumer] Event received: " << evt.component << " ["
              << severityToString(evt.severity) << "] " << evt.message << std::endl;
}

Statistics Consumer::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

// ---------- Consumer private methods ----------

void Consumer::printStatistics() const {
    Statistics statsCopy = getStatistics();

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "EVENT STATISTICS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "Total WARNING events: " << statsCopy.warningCount << std::endl;
    std::cout << "Total ERROR events:   " << statsCopy.errorCount << std::endl;
    std::cout << "\nBreakdown by Component:" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    for (const auto& [component, severity_map] : statsCopy.componentSeverityCount) {
        std::cout << "  [" << component << "]" << std::endl;
        for (const auto& [severity, count] : severity_map) {
            std::cout << "    " << std::setw(10) << severity << ": " << count << std::endl;
        }
    }

    std::cout << std::string(70, '=') << "\n" << std::endl;
}

// ---------- free function ----------

void runConsumer(const std::string& pipeName, IIPCFactory& factory) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Consumer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        Consumer consumer(pipeName, factory);
        installAppSignalHandler(&consumer);
        consumer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
