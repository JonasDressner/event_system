#include "consumer.hpp"
#include <iostream>
#include <csignal>

static Consumer* g_consumer = nullptr;

static void signalHandler(int signal) {
    std::cout << "\n[Consumer] Received shutdown signal. Stopping..." << std::endl;
    if (g_consumer) {
        g_consumer->stop();
    }
}

void runConsumer(const std::string& pipeName) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Consumer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        Consumer consumer(pipeName);
        g_consumer = &consumer;
        consumer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
