#include "producer.hpp"
#include <iostream>
#include <csignal>

static Producer* g_producer = nullptr;

static void signalHandler(int signal) {
    std::cout << "\n[Producer] Received shutdown signal. Stopping..." << std::endl;
    if (g_producer) {
        g_producer->stop();
    }
}

void runProducer(const std::string& pipeName) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Producer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        Producer producer(pipeName);
        g_producer = &producer;
        producer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
