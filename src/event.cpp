#include "event.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>

std::string severityToString(Severity sev) {
    switch (sev) {
        case Severity::INFO:       return "INFO";
        case Severity::WARNING:    return "WARNING";
        case Severity::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

Severity stringToSeverity(const std::string& str) {
    if (str == "WARNING") return Severity::WARNING;
    if (str == "ERROR")   return Severity::ERROR;
    return Severity::INFO;
}

std::string timestampToString(const std::chrono::system_clock::time_point& tp) {
    auto ms      = std::chrono::duration_cast<std::chrono::milliseconds>(
                       tp.time_since_epoch()) % 1000;
    auto t       = std::chrono::system_clock::to_time_t(tp);
    std::tm* tmi = std::localtime(&t);

    std::ostringstream ss;
    ss << std::put_time(tmi, "%H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}