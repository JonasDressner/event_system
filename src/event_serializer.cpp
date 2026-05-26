#include "event.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace EventSerializerUtils {

static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm_info = std::localtime(&time_t);
    
    std::ostringstream ss;
    ss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

static std::string escapeJson(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;
        }
    }
    return result;
}

static std::string extractValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

std::string toJson(const Event& e) {
    std::ostringstream ss;
    ss << "{\"timestamp\":\"" << formatTimestamp(e.timestamp)
       << "\",\"component\":\"" << escapeJson(e.component)
       << "\",\"severity\":\"" << severityToString(e.severity)
       << "\",\"message\":\"" << escapeJson(e.message) << "\"}";
    return ss.str();
}

Event fromJson(const std::string& json) {
    Event e;
    e.timestamp = std::chrono::system_clock::now();
    e.component = extractValue(json, "component");
    e.severity = stringToSeverity(extractValue(json, "severity"));
    e.message = extractValue(json, "message");
    return e;
}

}  // namespace EventSerializerUtils