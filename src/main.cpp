#include "iipc_factory.hpp"
#include "ipc_factory.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>

void runProducer(const std::string& pipeName, IIPCFactory& factory);
void runConsumer(const std::string& pipeName, IIPCFactory& factory);

// ---------------------------------------------------------------
// Factory selection — add new transports here only
// ---------------------------------------------------------------
std::unique_ptr<IIPCFactory> createFactory(const std::string& transport) {
    if (transport == "pipe") return std::make_unique<PipeIPCFactory>();
    // future: if (transport == "tcp") return std::make_unique<TCPIPCFactory>();
    // future: if (transport == "shm") return std::make_unique<SharedMemIPCFactory>();
    throw std::invalid_argument("Unknown transport: '" + transport + "'. Available: pipe");
}

// ---------------------------------------------------------------
// Argument parsing
// ---------------------------------------------------------------
enum class Mode { PRODUCER, CONSUMER, INVALID };

struct AppConfig {
    Mode        mode      = Mode::INVALID;
    std::string pipeName  = "event_pipe";
    std::string transport = "pipe";
};

AppConfig parseArgs(int argc, char* argv[]) {
    AppConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--producer") {
            cfg.mode = Mode::PRODUCER;
        } else if (arg == "--consumer") {
            cfg.mode = Mode::CONSUMER;
        } else if (arg == "--transport" && i + 1 < argc) {
            cfg.transport = argv[++i];
        } else if (arg.rfind("--", 0) != 0) {
            cfg.pipeName = arg;   // positional: pipe name
        }
    }

    return cfg;
}

// ---------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: EventSystem --producer|--consumer"
                     " [--transport pipe] [pipe_name]\n";
        return 1;
    }

    AppConfig cfg = parseArgs(argc, argv);

    if (cfg.mode == Mode::INVALID) {
        std::cerr << "Error: specify --producer or --consumer\n";
        return 1;
    }

    std::unique_ptr<IIPCFactory> factory;
    try {
        factory = createFactory(cfg.transport);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (cfg.mode == Mode::PRODUCER) {
        runProducer(cfg.pipeName, *factory);
    } else {
        runConsumer(cfg.pipeName, *factory);
    }

    return 0;
}
