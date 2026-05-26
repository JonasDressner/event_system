#pragma once

#include <string>
#include <chrono>

enum class Severity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2
};

std::string severityToString(Severity sev);
Severity stringToSeverity(const std::string& str);

struct Event {
    std::chrono::system_clock::time_point timestamp;
    std::string component;
    Severity severity{Severity::INFO};
    std::string message;
};