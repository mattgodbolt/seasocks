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

#include "seasocks/util/Html.h"

#include <gmock/gmock.h>

using namespace seasocks;
using namespace seasocks::html;

TEST(HtmlTests, shouldRenderASimpleDocument) {
    auto document = html::html(head(title("Hello")), body(h1("A header"), "This is a document"));
    EXPECT_EQ("<html><head><title>Hello</title></head><body><h1>A header</h1>This is a document</body></html>", document.str());
}

TEST(HtmlTests, shouldAllowAppending) {
    auto list = ul();
    list << li("Element 1") << li("Element 2");
    EXPECT_EQ("<ul><li>Element 1</li><li>Element 2</li></ul>", list.str());
}

TEST(HtmlTests, shouldAllowCommaSeparation) {
    auto list = ul(li("Element 1"), li("Element 2"));
    EXPECT_EQ("<ul><li>Element 1</li><li>Element 2</li></ul>", list.str());
}

TEST(HtmlTests, shouldHandleAttributes) {
    auto elem = span("Some text").clazz("class");
    EXPECT_EQ("<span class=\"class\">Some text</span>", elem.str());
}

TEST(HtmlTests, shouldAppendLikeAStreamFromEmpty) {
    auto elem = empty() << "Text " << 1 << ' ' << 10.23;
    EXPECT_EQ("Text 1 10.23", elem.str());
}

TEST(HtmlTests, shouldNotNeedExtraMarkupForTextNodes) {
    auto elem = text("This ") << text("is") << text(" a test") << ".";
    EXPECT_EQ("This is a test.", elem.str());
}

TEST(HtmlTests, shouldWorkWithAnchors) {
    auto elem = a("http://google.com/?q=badgers", div(span("foo")));
    EXPECT_EQ("<a href=\"http://google.com/?q=badgers\"><div><span>foo</span></div></a>", elem.str());
}

TEST(HtmlTests, shouldAddAll) {
    std::vector<std::string> strings = { "Hi", "Moo", "Foo" };
    auto list = ul().addAll(strings, [](const std::string& s) { return li(s); });
    EXPECT_EQ("<ul><li>Hi</li><li>Moo</li><li>Foo</li></ul>", list.str());

}
