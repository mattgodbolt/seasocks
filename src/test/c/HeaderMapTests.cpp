#include "internal/HeaderMap.h"

#include <gmock/gmock.h>

using namespace seasocks;

namespace {

TEST(HeaderMapTests, shouldConstruct) {
    HeaderMap map;
    EXPECT_TRUE(map.empty());
}

TEST(HeaderMapTests, shouldStoreAndRetrieve) {
    HeaderMap map;
    map.emplace("Foo", "Bar");
    EXPECT_EQ(1, map.size());
    EXPECT_EQ("Bar", map.at("Foo"));
    map.emplace("Baz", "Moo");
    EXPECT_EQ(2, map.size());
    EXPECT_EQ("Bar", map.at("Foo"));
    EXPECT_EQ("Moo", map.at("Baz"));
}

TEST(HeaderMapTests, shouldBeCaseInsensitive) {
    HeaderMap map;
    map.emplace("Foo", "Bar");
    EXPECT_EQ("Bar", map.at("FOO"));
    EXPECT_EQ("Bar", map.at("foO"));
}

TEST(HeaderMapTests, shouldPreserveOriginalCase) {
    HeaderMap map;
    map.emplace("Foo", "Bar");
    auto it = map.find("Foo");
    EXPECT_EQ("Foo", it->first);
}

}
