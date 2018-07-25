
#include "seasocks/ResponseBuilder.h"
#include <catch2/catch.hpp>

using namespace seasocks;
using namespace Catch::Matchers;

TEST_CASE("appendIntValue", "[ResponseBuilderTests]") {
    ResponseBuilder builder;
    builder << int{3};
    auto result = std::dynamic_pointer_cast<SynchronousResponse>(builder.build());
    CHECK_THAT(result->payload(), Equals("3"));
}

TEST_CASE("appendCStringValue", "[ResponseBuilderTests]") {
    ResponseBuilder builder;
    const char* cStr = "abc";
    builder << cStr;
    auto result = std::dynamic_pointer_cast<SynchronousResponse>(builder.build());
    CHECK_THAT(result->payload(), Equals("abc"));
}

TEST_CASE("appendStringValue", "[ResponseBuilderTests]") {
    ResponseBuilder builder;
    builder << std::string("xyz");
    auto result = std::dynamic_pointer_cast<SynchronousResponse>(builder.build());
    CHECK_THAT(result->payload(), Equals("xyz"));
}
