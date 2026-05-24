#pragma once

#include "event.hpp"
#include "ipc.hpp"
#include <random>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>

class Producer {
public:
    Producer(const std::string& pipeName = "event_pipe") : writer(pipeName), running(true) {
        components = {"Database", "API", "Cache", "Auth", "Logging"};
        severity_dist = std::uniform_int_distribution<>(0, 2);
    }

    void start() {
        std::cout << "[Producer] Starting event generation..." << std::endl;
        
        int eventCount = 0;
        while (running) {
            try {
                Event evt = generateEvent();
                std::string serialized = evt.serialize();
                writer.write(serialized);
                
                std::cout << "[Producer] Event #" << (++eventCount) << " sent: " 
                          << evt.component << " [" << severityToString(evt.severity) << "]" 
                          << std::endl;
                
                // Generate events every 500ms
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } catch (const std::exception& e) {
                std::cerr << "[Producer] Error: " << e.what() << std::endl;
                break;
            }
        }
    }

    void stop() {
        running = false;
    }

private:
    IPCWriter writer;
    bool running;
    std::vector<std::string> components;
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> severity_dist;
    std::uniform_int_distribution<> component_dist{0, 4};
    std::uniform_int_distribution<> message_dist{0, 4};

    Event generateEvent() {
        Event evt;
        evt.timestamp = std::chrono::system_clock::now();
        evt.component = components[component_dist(gen)];
        evt.severity = static_cast<Severity>(severity_dist(gen));
        
        // Generate message based on severity
        std::string messages[] = {
            "Operation completed successfully",
            "Processing request",
            "Performance degradation detected",
            "Connection timeout",
            "Critical system error"
        };
        evt.message = messages[message_dist(gen)];
        
        return evt;
    }
};
