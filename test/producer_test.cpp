#include "fakes.hpp"
#include "producer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <set>
#include <string>

TEST_CASE("Producer generates events", "[Producer]") {
    auto fakeWriter = std::make_unique<FakeWriter>();
    auto fakeSerializer = std::make_shared<FakeSerializer>();

    Producer producer(std::move(fakeWriter), fakeSerializer);
    Event evt = producer.generateEvent();

    REQUIRE(!evt.component.empty());
    REQUIRE(evt.severity >= Severity::INFO);
    REQUIRE(evt.severity <= Severity::ERROR);
}

TEST_CASE("Producer component is always from the known table", "[Producer]") {
    // Mirrors Producer::kComponents — if the table changes, this test catches it.
    static const std::set<std::string> kValidComponents = {
        "Database", "API", "Cache", "Auth", "Logging"};

    auto fakeWriter = std::make_unique<FakeWriter>();
    auto fakeSerializer = std::make_shared<FakeSerializer>();
    Producer producer(std::move(fakeWriter), fakeSerializer);

    // Generate enough events to exercise all distribution values.
    for (int i = 0; i < 20; ++i) {
        Event evt = producer.generateEvent();
        REQUIRE(kValidComponents.count(evt.component) == 1);
        REQUIRE(!evt.message.empty());
    }
}

TEST_CASE("Producer event timestamp is within generation window", "[Producer]") {
    auto fakeWriter = std::make_unique<FakeWriter>();
    auto fakeSerializer = std::make_shared<FakeSerializer>();
    Producer producer(std::move(fakeWriter), fakeSerializer);

    auto before = std::chrono::system_clock::now();
    Event evt = producer.generateEvent();
    auto after = std::chrono::system_clock::now();

    REQUIRE(evt.timestamp >= before);
    REQUIRE(evt.timestamp <= after);
}
