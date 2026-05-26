#include <catch2/catch_test_macros.hpp>
#include "event.hpp"

TEST_CASE("severityToString and stringToSeverity", "[Severity]") {
    REQUIRE(severityToString(Severity::INFO) == "INFO");
    REQUIRE(severityToString(Severity::WARNING) == "WARNING");
    REQUIRE(severityToString(Severity::ERROR_LEVEL) == "ERROR");
    REQUIRE(stringToSeverity("WARNING") == Severity::WARNING);
    REQUIRE(stringToSeverity("ERROR") == Severity::ERROR_LEVEL);
    REQUIRE(stringToSeverity("UNKNOWN") == Severity::INFO);
}

TEST_CASE("Event serialize and deserialize preserve component, severity, message", "[Event]") {
    Event evt;
    evt.timestamp = std::chrono::system_clock::now();
    evt.component = "TestComponent";
    evt.severity = Severity::ERROR_LEVEL;
    evt.message = "Test message";

    std::string serialized = evt.serialize();
    Event parsed = Event::deserialize(serialized);

    REQUIRE(parsed.component == "TestComponent");
    REQUIRE(parsed.severity == Severity::ERROR_LEVEL);
    REQUIRE(parsed.message == "Test message");
}

TEST_CASE("Event deserialize missing values uses defaults", "[Event]") {
    std::string data = R"({"timestamp":"2025-01-01 00:00:00.000","component":"","severity":"","message":""})";
    Event parsed = Event::deserialize(data);

    REQUIRE(parsed.component == "");
    REQUIRE(parsed.severity == Severity::INFO);
    REQUIRE(parsed.message == "");
}