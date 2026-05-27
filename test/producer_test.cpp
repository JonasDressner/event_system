#include "fakes.hpp"
#include "producer.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Producer generates events", "[Producer]") {
    auto fakeWriter = std::make_unique<FakeWriter>();
    auto fakeSerializer = std::make_shared<FakeSerializer>();

    Producer producer(std::move(fakeWriter), fakeSerializer);
    Event evt = producer.generateEvent();

    REQUIRE(!evt.component.empty());
    REQUIRE(evt.severity >= Severity::INFO);
    REQUIRE(evt.severity <= Severity::ERROR);
}