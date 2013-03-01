#include "seasocks/util/Json.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace seasocks;

namespace {

TEST(JsonTests, shouldHandleMaps) {
    EXPECT_STREQ("{\"monkey\":12}", makeMap("monkey", 12).c_str());
}

TEST(JsonTests, shouldHandleQuotedStrings) {
    EXPECT_STREQ("{\"key\":\"I have \\\"quotes\\\"\"}",
            makeMap("key", "I have \"quotes\"").c_str());
}

}
