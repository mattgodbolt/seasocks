#include "internal/HeaderMap.h"

#include "internal/Config.h"

#include <gmock/gmock.h>

using namespace seasocks;

namespace {

void emplace(HeaderMap &map, const char *header, const char *value) {
#if HAVE_UNORDERED_MAP_EMPLACE
    map.emplace(header, value);
#else
    map.insert(std::make_pair(header, value));
#endif
}

TEST(HeaderMapTests, shouldConstruct) {
    HeaderMap map;
    EXPECT_TRUE(map.empty());
}

TEST(HeaderMapTests, shouldStoreAndRetrieve) {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    EXPECT_EQ(1, map.size());
    EXPECT_EQ("Bar", map.at("Foo"));
    emplace(map, "Baz", "Moo");
    EXPECT_EQ(2, map.size());
    EXPECT_EQ("Bar", map.at("Foo"));
    EXPECT_EQ("Moo", map.at("Baz"));
}

TEST(HeaderMapTests, shouldBeCaseInsensitive) {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    EXPECT_EQ("Bar", map.at("FOO"));
    EXPECT_EQ("Bar", map.at("foO"));
}

TEST(HeaderMapTests, shouldPreserveOriginalCase) {
    HeaderMap map;
    emplace(map, "Foo", "Bar");
    auto it = map.find("Foo");
    EXPECT_EQ("Foo", it->first);
}

}
