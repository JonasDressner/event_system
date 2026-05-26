#include "producer.hpp"
#include "signal_helper.hpp"
#include <iostream>
#include <csignal>

void runProducer(const std::string& pipeName) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Producer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        Producer producer(pipeName);
        installAppSignalHandler(&producer);
        producer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
