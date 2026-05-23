#include "producer.hpp"
#include "consumer.hpp"
#include <iostream>
#include <string>

void runProducer(const std::string& pipeName);
void runConsumer(const std::string& pipeName);

int main(int argc, char* argv[]) {
    std::string mode;
    std::string pipeName = "event_pipe";

    if (argc < 2) {
        std::cout << "Usage: event_system [--producer|--consumer] [pipe_name]" << std::endl;
        return 1;
    }

    mode = argv[1];
    if (argc >= 3) {
        pipeName = argv[2];
    }

    if (mode == "--producer") {
        runProducer(pipeName);
    } else if (mode == "--consumer") {
        runConsumer(pipeName);
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        std::cerr << "Usage: event_system [--producer|--consumer] [pipe_name]" << std::endl;
        return 1;
    }

    return 0;
}
