#pragma once

#include "event.hpp"
#include "ipc.hpp"
#include <map>
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>

struct Statistics {
    int warning_count = 0;
    int error_count = 0;
    std::map<std::string, std::map<std::string, int>> component_severity_count;
};

class Consumer {
public:
    Consumer(const std::string& pipeName = "event_pipe") 
        : reader(pipeName), running(true), lastStatsTime(std::chrono::steady_clock::now()) {
    }

    void start() {
        std::cout << "[Consumer] Starting event processing..." << std::endl;
        
        while (running) {
            try {
                std::string message;
                if (reader.read(message, 100)) {
                    Event evt = Event::deserialize(message);
                    processEvent(evt);
                }
                
                // Print statistics every 5 seconds
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime);
                if (elapsed.count() >= 5) {
                    printStatistics();
                    lastStatsTime = now;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (const std::exception& e) {
                std::cerr << "[Consumer] Error: " << e.what() << std::endl;
                break;
            }
        }
    }

    void stop() {
        running = false;
    }

private:
    IPCReader reader;
    bool running;
    Statistics stats;
    std::chrono::steady_clock::time_point lastStatsTime;

    void processEvent(const Event& evt) {
        // Filter: only process WARNING and ERROR events
        if (evt.severity < Severity::WARNING) {
            return; // Ignore INFO events
        }

        // Log the event
        std::cout << "[Consumer] Event received: " << evt.component << " [" 
                  << severityToString(evt.severity) << "] " << evt.message << std::endl;

        // Update statistics
        updateStatistics(evt);
    }

    void updateStatistics(const Event& evt) {
        if (evt.severity == Severity::WARNING) {
            stats.warning_count++;
        } else if (evt.severity == Severity::ERROR_LEVEL) {
            stats.error_count++;
        }

        // Aggregate by component and severity
        std::string severity_str = severityToString(evt.severity);
        stats.component_severity_count[evt.component][severity_str]++;
    }

    void printStatistics() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "EVENT STATISTICS" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        std::cout << "Total WARNING events: " << stats.warning_count << std::endl;
        std::cout << "Total ERROR events:   " << stats.error_count << std::endl;
        
        std::cout << "\nBreakdown by Component:" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        for (const auto& [component, severity_map] : stats.component_severity_count) {
            std::cout << "  [" << component << "]" << std::endl;
            for (const auto& [severity, count] : severity_map) {
                std::cout << "    " << std::setw(10) << severity << ": " << count << std::endl;
            }
        }
        
        std::cout << std::string(70, '=') << "\n" << std::endl;
    }
};
