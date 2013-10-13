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
