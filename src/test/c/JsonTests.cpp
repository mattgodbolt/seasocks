// Copyright (c) 2013-2017, Matt Godbolt
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

#include <catch2/catch.hpp>

#include <ostream>
#include <map>
#include <unordered_map>

using namespace seasocks;

namespace {

TEST_CASE("shouldHandleMaps", "[JsonTests]") {
    CHECK(makeMap("monkey", 12) == "{\"monkey\":12}");
}

TEST_CASE("shouldHandleQuotedStrings", "[JsonTests]") {
    CHECK(makeMap("key", "I have \"quotes\"") ==
          "{\"key\":\"I have \\\"quotes\\\"\"}");
}

TEST_CASE("shouldHandleNewLinesInStrings", "[JsonTests]") {
    std::stringstream str;
    jsonToStream(str, "I have\nnew\rlines");
    CHECK(str.str() == "\"I have\\nnew\\rlines\"");
}

TEST_CASE("shouldHandleAsciiStrings", "[JsonTests]") {
    std::stringstream str;
    jsonToStream(str, "0123xyz!$%(/)=_-");
    CHECK(str.str() == R"("0123xyz!$%(/)=_-")");
}

TEST_CASE("shouldHandleCrazyChars", "[JsonTests]") {
    std::stringstream str;
    jsonToStream(str, "\x01\x02\x1f");
    CHECK(str.str() == "\"\\u0001\\u0002\\u001f\"");
}

TEST_CASE("shouldHandleDate", "[JsonTests]") {
    std::stringstream str;
    jsonToStream(str, EpochTimeAsLocal(209001600));
    CHECK(str.str() == "new Date(209001600000).toLocaleString()");
}

TEST_CASE("shouldHandleNonAsciiChars", "[JsonTests]") {
    std::stringstream str;
    jsonToStream(str, "§");
    CHECK(str.str() == R"("§")");
}

struct Object {
    void jsonToStream(std::ostream& ostr) const {
        ostr << makeMap("object", true);
    }
    // Clang is pernickity about this. We don't want use this function
    // but it's here to catch errors where we accidentally use it instead of the
    // jsonToStream.
    friend std::ostream& operator<<(std::ostream& o, const Object&) {
        return o << "Not this one";
    }
};

struct Object2 {
    friend std::ostream& operator<<(std::ostream& o, const Object2&) {
        return o << "This is object 2";
    }
};

static_assert(is_streamable<Object2>::value, "Should be streamable");

TEST_CASE("shouldHandleCustomObjects", "[JsonTests]") {
    CHECK(makeMap("obj", Object()) == R"({"obj":{"object":true}})");
    Object o;
    CHECK(makeMap("obj", o) == R"({"obj":{"object":true}})");
    CHECK(makeMap("obj", Object2()) == R"({"obj":"This is object 2"})");
    Object2 o2;
    CHECK(makeMap("obj", o2) == R"({"obj":"This is object 2"})");
    // Putting a clang-specific pragma to thwart the unused warning in Object
    // upsets GCC...so we just put a test for this to ensure it's used.
    std::ostringstream ost;
    ost << Object();
    CHECK(ost.str() == "Not this one"); // See comment
}

TEST_CASE("to_json", "[JsonTests]") {
    CHECK(to_json(1) == "1");
    CHECK(to_json(3.14) == "3.14");
    CHECK(to_json("hello") == "\"hello\"");
    CHECK(to_json(Object()) == R"({"object":true})");
    CHECK(to_json(Object2()) == R"("This is object 2")");
}

TEST_CASE("handlesArrays", "[JsonTests]") {
    CHECK(makeArray() == R"([])");
    CHECK(makeArray(1) == R"([1])");
    CHECK(makeArray(1, 2, 3) == R"([1,2,3])");
    CHECK(makeArray({1, 2, 3}) == R"([1,2,3])");
    CHECK(makeArray("abc") == R"(["abc"])");
    CHECK(makeArray("a", "b", "c") == R"(["a","b","c"])");
    CHECK(makeArray({"a", "b", "c"}) == R"(["a","b","c"])");
    std::vector<JsonnedString> strs = {to_json(false), to_json(true)};
    CHECK(makeArrayFromContainer(strs) == R"([false,true])");
}

TEST_CASE("handlesMaps", "[JsonTests]") {
    using namespace Catch::Matchers;

    std::map<std::string, JsonnedString> ordMap;
    ordMap["hello"] = to_json(true);
    ordMap["goodbye"] = to_json(false);
    CHECK(makeMapFromContainer(ordMap) == R"({"goodbye":false,"hello":true})");
    std::map<std::string, JsonnedString> unordMap;
    unordMap["hello"] = to_json(true);
    unordMap["goodbye"] = to_json(false);
    CHECK_THAT(makeMapFromContainer(unordMap), Catch::Equals(R"({"goodbye":false,"hello":true})") || Catch::Equals(R"({"hello":true,"goodbye":false})"));
}

}
