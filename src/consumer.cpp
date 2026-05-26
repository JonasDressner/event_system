#include "consumer.hpp"
#include "signal_helper.hpp"
#include <iostream>
#include <csignal>

void runConsumer(const std::string& pipeName) {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "Event System - Consumer" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        Consumer consumer(pipeName);
        installAppSignalHandler(&consumer);
        consumer.start();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
}
