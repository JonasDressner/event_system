#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

enum class Severity {
    INFO = 0,
    WARNING = 1,
    ERROR_LEVEL = 2
};

inline std::string severityToString(Severity sev) {
    switch (sev) {
        case Severity::INFO:       return "INFO";
        case Severity::WARNING:    return "WARNING";
        case Severity::ERROR_LEVEL: return "ERROR";
        default:                   return "UNKNOWN";
    }
}

inline Severity stringToSeverity(const std::string& str) {
    if (str == "WARNING") return Severity::WARNING;
    if (str == "ERROR")   return Severity::ERROR_LEVEL;
    return Severity::INFO;
}

struct Event {
    std::chrono::system_clock::time_point timestamp;
    std::string component;
    Severity severity;
    std::string message;

    // Serialize to string (JSON format)
    std::string serialize() const {
        std::stringstream ss;
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
        
        ss << "{\"timestamp\":\"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        ss << "\",\"component\":\"" << component << "\",\"severity\":\"" 
           << severityToString(severity) << "\",\"message\":\"" << message << "\"}";
        return ss.str();
    }

    // Deserialize from string (JSON format)
    static Event deserialize(const std::string& data) {
        Event evt;
        evt.timestamp = std::chrono::system_clock::now();
        evt.component = "UNKNOWN";
        evt.severity = Severity::INFO;
        evt.message = "";

        // Simple JSON parsing
        size_t pos = 0;
        
        // Extract component
        size_t start = data.find("\"component\":\"") + 13;
        size_t end = data.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            evt.component = data.substr(start, end - start);
        }

        // Extract severity
        start = data.find("\"severity\":\"") + 12;
        end = data.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            evt.severity = stringToSeverity(data.substr(start, end - start));
        }

        // Extract message
        start = data.find("\"message\":\"") + 11;
        end = data.find("\"", start);
        if (start != std::string::npos && end != std::string::npos) {
            evt.message = data.substr(start, end - start);
        }

        return evt;
    }
};
