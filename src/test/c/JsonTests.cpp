// Copyright (c) 2013, Matt Godbolt
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this 
// list of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "seasocks/util/Json.h"

#include <gmock/gmock.h>
#include <ostream>
#include <map>
#include <unordered_map>

using namespace testing;
using namespace seasocks;

namespace {

TEST(JsonTests, shouldHandleMaps) {
    EXPECT_EQ("{\"monkey\":12}", makeMap("monkey", 12));
}

TEST(JsonTests, shouldHandleQuotedStrings) {
    EXPECT_EQ("{\"key\":\"I have \\\"quotes\\\"\"}",
            makeMap("key", "I have \"quotes\""));
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

TEST(JsonTests, shouldHandleDate) {
    std::stringstream str;
    jsonToStream(str, EpochTimeAsLocal(209001600));
    EXPECT_EQ("new Date(209001600000).toLocaleString()", str.str());
}

struct Object {
    void jsonToStream(std::ostream &ostr) const {
        ostr << makeMap("object", true);
    }
    // Clang is pernickity about this. We don't want use this function
    // but it's here to catch errors where we accidentally use it instead of the
    // jsonToStream.
    friend std::ostream &operator << (std::ostream &o, const Object &ob) {
        return o << "Not this one";
    }
};

struct Object2 {
    friend std::ostream &operator << (std::ostream &o, const Object2 &ob) {
        return o << "This is object 2";
    }
};

static_assert(is_streamable<Object2>::value, "Should be streamable");

TEST(JsonTests, shouldHandleCustomObjects) {
    EXPECT_EQ(R"({"obj":{"object":true}})", makeMap("obj", Object()));
    Object o;
    EXPECT_EQ(R"({"obj":{"object":true}})", makeMap("obj", o));
    EXPECT_EQ(R"({"obj":"This is object 2"})", makeMap("obj", Object2()));
    Object2 o2;
    EXPECT_EQ(R"({"obj":"This is object 2"})", makeMap("obj", o2));
    // Putting a clang-specific pragma to thwart the unused warning in Object
    // upsets GCC...so we just put a test for this to ensure it's used.
    std::ostringstream ost;
    ost << Object();
    EXPECT_EQ("Not this one", ost.str()); // See comment
}

TEST(JsonTests, to_json) {
    EXPECT_EQ("1", to_json(1));
    EXPECT_EQ("3.14", to_json(3.14));
    EXPECT_EQ("\"hello\"", to_json("hello"));
    EXPECT_EQ(R"({"object":true})", to_json(Object()));
    EXPECT_EQ(R"("This is object 2")", to_json(Object2()));
}
TEST(JsonTests, handlesArrays) {
    EXPECT_EQ(R"([])", makeArray());
    EXPECT_EQ(R"([1])", makeArray(1));
    EXPECT_EQ(R"([1,2,3])", makeArray(1, 2, 3));
    EXPECT_EQ(R"([1,2,3])", makeArray({1, 2, 3}));
    EXPECT_EQ(R"(["abc"])", makeArray("abc"));
    EXPECT_EQ(R"(["a","b","c"])", makeArray("a", "b", "c"));
    EXPECT_EQ(R"(["a","b","c"])", makeArray({"a", "b", "c"}));
    std::vector<JsonnedString> strs = { to_json(false), to_json(true) };
    EXPECT_EQ(R"([false,true])", makeArrayFromContainer(strs));
}

TEST(JsonTests, handlesMaps) {
    std::map<std::string, JsonnedString> ordMap;
    ordMap["hello"] = to_json(true);
    ordMap["goodbye"] = to_json(false);
    EXPECT_EQ(R"({"goodbye":false,"hello":true})", makeMapFromContainer(ordMap));
    std::map<std::string, JsonnedString> unordMap;
    unordMap["hello"] = to_json(true);
    unordMap["goodbye"] = to_json(false);
    EXPECT_THAT(makeMapFromContainer(unordMap), AnyOf(
            Eq(R"({"goodbye":false,"hello":true})"),
            Eq(R"({"hello":true,"goodbye":false})")));
}

}
