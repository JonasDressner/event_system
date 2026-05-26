#include "event.hpp"
#include <stdexcept>

std::string severityToString(Severity sev) {
    switch (sev) {
        case Severity::INFO:       return "INFO";
        case Severity::WARNING:    return "WARNING";
        case Severity::ERROR_LEVEL: return "ERROR";
    }
    return "UNKNOWN";
}

Severity stringToSeverity(const std::string& str) {
    if (str == "WARNING") return Severity::WARNING;
    if (str == "ERROR")   return Severity::ERROR_LEVEL;
    return Severity::INFO;
}