#include "event.hpp"
#include "event_serializer.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Event serialization", "[Event]") {
    Event evt;
    evt.timestamp = std::chrono::system_clock::now();
    evt.component = "Database";
    evt.severity = Severity::WARNING;
    evt.message = "Connection timeout";

    std::string serialized = EventSerializerUtils::toJson(evt);
    Event parsed = EventSerializerUtils::fromJson(serialized);

    REQUIRE(parsed.component == "Database");
    REQUIRE(parsed.severity == Severity::WARNING);
    REQUIRE(parsed.message == "Connection timeout");

    // Timestamp round-trip: serialized with second precision → difference < 1 s
    auto diffMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(parsed.timestamp - evt.timestamp)
            .count();
    REQUIRE(std::abs(diffMs) < 1000);
}

TEST_CASE("Event deserialization", "[Event]") {
    std::string data =
        R"({"timestamp":"2026-05-26 10:30:45.123","component":"API","severity":"ERROR","message":"Internal server error"})";
    Event parsed = EventSerializerUtils::fromJson(data);

    REQUIRE(parsed.component == "API");
    REQUIRE(parsed.severity == Severity::ERROR);
    REQUIRE(parsed.message == "Internal server error");
}

TEST_CASE("Event severity conversion", "[Event]") {
    REQUIRE(severityToString(Severity::INFO) == "INFO");
    REQUIRE(severityToString(Severity::WARNING) == "WARNING");
    REQUIRE(severityToString(Severity::ERROR) == "ERROR");

    REQUIRE(stringToSeverity("INFO") == Severity::INFO);
    REQUIRE(stringToSeverity("WARNING") == Severity::WARNING);
    REQUIRE(stringToSeverity("ERROR") == Severity::ERROR);
}