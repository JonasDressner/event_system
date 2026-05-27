#include "producer.hpp"
#include "event_serializer.hpp"
#include "iipc_factory.hpp"
#include "signal_helper.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

// ---------- Producer constructors ----------

Producer::Producer(std::unique_ptr<IIPCWriter> writer,
                   std::shared_ptr<IEventSerializer> serializer)
    : writer_(std::move(writer))
    , serializer_(std::move(serializer))
    , running_(true)
    , gen_(std::random_device{}())
    , severity_dist_(0, 2)
    , component_dist_(0, 4)
    , message_dist_(0, 4)
{
    components_ = {"Database", "API", "Cache", "Auth", "Logging"};
}

Producer::Producer(const std::string& pipeName, IIPCFactory& factory)
    : Producer(factory.createWriter(pipeName), std::make_shared<SimpleEventSerializer>())
{}

// ---------- Producer public methods ----------

Event Producer::generateEvent() {
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

void Producer::start() {
    std::cout << "[Producer] Starting event generation..." << std::endl;
    int eventCount = 0;

    while (running_.load()) {
        try {
            Event evt = generateEvent();
            std::string serialized = serializer_->serialize(evt);
            writer_->write(serialized);

            std::cout << "[" << timestampToString(evt.timestamp) << "]"
                      << " [Producer] Event #" << (++eventCount)
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

void Producer::stop() noexcept {
    running_.store(false);
}

// ---------- free function ----------

void runProducer(const std::string& pipeName, IIPCFactory& factory) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Producer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        Producer producer(pipeName, factory);
        installAppSignalHandler(&producer);
        producer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
