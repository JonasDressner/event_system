#include "consumer.hpp"
#include "fakes.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Consumer filters INFO events - no statistics update", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "API";
    evt.severity = Severity::INFO;
    evt.message = "All good";
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warningCount == 0);
    REQUIRE(stats.errorCount == 0);
    REQUIRE(stats.componentSeverityCount.empty());
}

TEST_CASE("Consumer counts WARNING events", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "Cache";
    evt.severity = Severity::WARNING;
    evt.message = "Cache miss";
    consumer.processEvent(evt);
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warningCount == 2);
    REQUIRE(stats.errorCount == 0);
    REQUIRE(stats.componentSeverityCount.at("Cache").at("WARNING") == 2);
}

TEST_CASE("Consumer counts ERROR events", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event evt;
    evt.component = "Database";
    evt.severity = Severity::ERROR;
    evt.message = "Connection timeout";
    consumer.processEvent(evt);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warningCount == 0);
    REQUIRE(stats.errorCount == 1);
    REQUIRE(stats.componentSeverityCount.at("Database").at("ERROR") == 1);
}

TEST_CASE("Consumer tracks WARNING and ERROR for the same component", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event warn;
    warn.component = "API";
    warn.severity = Severity::WARNING;
    warn.message = "Slow response";

    Event err;
    err.component = "API";
    err.severity = Severity::ERROR;
    err.message = "Request failed";

    consumer.processEvent(warn);
    consumer.processEvent(err);
    consumer.processEvent(warn);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warningCount == 2);
    REQUIRE(stats.errorCount == 1);
    REQUIRE(stats.componentSeverityCount.at("API").at("WARNING") == 2);
    REQUIRE(stats.componentSeverityCount.at("API").at("ERROR") == 1);
    // Both severities live under the same component key — not two separate entries.
    REQUIRE(stats.componentSeverityCount.count("API") == 1);
}

TEST_CASE("Consumer tracks multiple components independently", "[Consumer]") {
    Consumer consumer(std::make_unique<FakeReader>(), std::make_shared<FakeSerializer>());

    Event warn;
    warn.component = "Cache";
    warn.severity = Severity::WARNING;
    warn.message = "Cache miss";

    Event err;
    err.component = "Database";
    err.severity = Severity::ERROR;
    err.message = "Connection timeout";

    consumer.processEvent(warn);
    consumer.processEvent(err);
    consumer.processEvent(warn);

    auto stats = consumer.getStatistics();
    REQUIRE(stats.warningCount == 2);
    REQUIRE(stats.errorCount == 1);
    REQUIRE(stats.componentSeverityCount.at("Cache").at("WARNING") == 2);
    REQUIRE(stats.componentSeverityCount.at("Database").at("ERROR") == 1);
    REQUIRE(stats.componentSeverityCount.count("Cache") == 1);
    REQUIRE(stats.componentSeverityCount.count("Database") == 1);
}
