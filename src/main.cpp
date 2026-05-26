#include "producer.hpp"
#include "consumer.hpp"
#include <iostream>
#include <string>

void runProducer(const std::string& pipeName);
void runConsumer(const std::string& pipeName);

enum class Mode { Producer, Consumer, Invalid };

Mode parseMode(const std::string& arg) {
    if (arg == "--producer") return Mode::Producer;
    if (arg == "--consumer") return Mode::Consumer;
    return Mode::Invalid;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: event_system [--producer|--consumer] [pipe_name]\n";
        return 1;
    }

    Mode mode = parseMode(argv[1]);
    std::string pipeName = (argc >= 3) ? argv[2] : "event_pipe";

    if (mode == Mode::Producer) {
        runProducer(pipeName);
    } else if (mode == Mode::Consumer) {
        runConsumer(pipeName);
    } else {
        std::cerr << "Unknown mode: " << argv[1] << "\n";
        return 1;
    }

    return 0;
}
