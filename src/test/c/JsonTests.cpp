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

TEST(JsonTests, shouldHandleNewLinesInStrings) {
    std::stringstream str;
    jsonToStream(str, "I have\nnew\rlines");
    EXPECT_EQ("\"I have\\nnew\\rlines\"", str.str());
}

TEST(JsonTests, shouldHandleCrazyChars) {
    std::stringstream str;
    jsonToStream(str, "\x01\x02\x1f");
    EXPECT_EQ("\"\\u0001\\u0002\\u001f\"", str.str());
}

}
