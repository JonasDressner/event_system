#pragma once

#include <csignal>
#include <atomic>
#include <iostream>

struct IStopper {
    virtual ~IStopper() = default;
    virtual void stop() noexcept = 0;
};

inline std::atomic<IStopper*> gAppInstance{nullptr};

inline void appSignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nCTRL+C received. Stop application...\n";
    } else if (signal == SIGTERM) {
        std::cout << "\nSIGTERM received. Stop application...\n";
    }
    auto* ptr = gAppInstance.load(std::memory_order_acquire);
    if (ptr) {
        ptr->stop();
    }
}

inline void installAppSignalHandler(IStopper* target) {
    gAppInstance.store(target, std::memory_order_release);
    std::signal(SIGINT, appSignalHandler);
    std::signal(SIGTERM, appSignalHandler);
}