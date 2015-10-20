#include "internal/HeaderMap.h"

#include "internal/Config.h"

#include "catch.hpp"

using namespace seasocks;

namespace {

void emplace(HeaderMap &map, const char *header, const char *value) {
#if HAVE_UNORDERED_MAP_EMPLACE
    map.emplace(header, value);
#else
    map.insert(std::make_pair(header, value));
#endif
}

TEST_CASE("shouldConstruct", "[HeaderMapTests]") {
    HeaderMap map;
    CHECK(map.empty());
}

TEST_CASE("shouldStoreAndRetrieve", "[HeaderMapTests]") {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    CHECK(map.size() == 1);
    CHECK(map.at("Foo") == "Bar");
    emplace(map, "Baz", "Moo");
    CHECK(map.size() == 2);
    CHECK(map.at("Foo") == "Bar");
    CHECK(map.at("Baz") == "Moo");
}

TEST_CASE("shouldBeCaseInsensitive", "[HeaderMapTests]") {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    CHECK(map.at("FOO") == "Bar");
    CHECK(map.at("foO") == "Bar");
}

TEST_CASE("shouldPreserveOriginalCase", "[HeaderMapTests]") {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    auto it = map.find("Foo");
    CHECK(it->first == "Foo");
}

}
