#include <catch2/catch_test_macros.hpp>
#include "consumer.hpp"
#include "fakes.hpp"

TEST_CASE("Consumer processes events", "[Consumer]") {
    auto fakeReader = std::make_unique<FakeReader>();
    auto fakeSerializer = std::make_shared<FakeSerializer>();
    
    Event e;
    e.component = "Cache";
    e.severity = Severity::ERROR_LEVEL;
    e.message = "Cache miss";
    
    fakeReader->to_send.push_back(EventSerializerUtils::toJson(e));
    
    Consumer consumer(std::move(fakeReader), fakeSerializer);
    Event recv = fakeSerializer->deserialize(EventSerializerUtils::toJson(e));
    
    REQUIRE(recv.component == "Cache");
    REQUIRE(recv.severity == Severity::ERROR_LEVEL);
}