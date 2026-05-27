#include <catch2/catch_test_macros.hpp>
#include "consumer.hpp"
#include "fakes.hpp"

TEST_CASE("Consumer filters INFO events — no statistics update", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "API";
    evt.severity  = Severity::INFO;
    evt.message   = "All good";
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warning_count == 0);
    REQUIRE(stats.error_count   == 0);
    REQUIRE(stats.component_severity_count.empty());
}

TEST_CASE("Consumer counts WARNING events", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "Cache";
    evt.severity  = Severity::WARNING;
    evt.message   = "Cache miss";
    consumer.processEvent(evt);
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warning_count == 2);
    REQUIRE(stats.error_count   == 0);
    REQUIRE(stats.component_severity_count.at("Cache").at("WARNING") == 2);
}

TEST_CASE("Consumer counts ERROR events", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "Database";
    evt.severity  = Severity::ERROR;
    evt.message   = "Connection timeout";
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warning_count == 0);
    REQUIRE(stats.error_count   == 1);
    REQUIRE(stats.component_severity_count.at("Database").at("ERROR") == 1);
}

TEST_CASE("Consumer tracks multiple components independently", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event warn;
    warn.component = "Cache";
    warn.severity  = Severity::WARNING;
    warn.message   = "Cache miss";

    Event err;
    err.component = "Database";
    err.severity  = Severity::ERROR;
    err.message   = "Connection timeout";

    consumer.processEvent(warn);
    consumer.processEvent(err);
    consumer.processEvent(warn);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warning_count == 2);
    REQUIRE(stats.error_count   == 1);
    REQUIRE(stats.component_severity_count.at("Cache").at("WARNING")    == 2);
    REQUIRE(stats.component_severity_count.at("Database").at("ERROR")   == 1);
    REQUIRE(stats.component_severity_count.count("Cache")  == 1);
    REQUIRE(stats.component_severity_count.count("Database") == 1);
}
