#pragma once

#include <csignal>
#include <atomic>

struct IStopper {
    virtual ~IStopper() = default;
    virtual void stop() noexcept = 0;
};

inline std::atomic<IStopper*> g_app_instance{nullptr};

inline void appSignalHandler(int) {
    auto ptr = g_app_instance.load(std::memory_order_acquire);
    if (ptr) {
        ptr->stop();
    }
}

inline void installAppSignalHandler(IStopper* target) {
    g_app_instance.store(target, std::memory_order_release);
    std::signal(SIGINT, appSignalHandler);
    std::signal(SIGTERM, appSignalHandler);
}