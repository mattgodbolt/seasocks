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

#include "seasocks/util/Html.h"

#include <catch2/catch.hpp>

using namespace seasocks;
using namespace seasocks::html;

TEST_CASE("shouldRenderASimpleDocument", "[HtmlTests]") {
    auto document = html::html(head(title("Hello")), body(h1("A header"), "This is a document"));
    CHECK(document.str() == "<html><head><title>Hello</title></head><body><h1>A header</h1>This is a document</body></html>");
}

TEST_CASE("shouldAllowAppending", "[HtmlTests]") {
    auto list = ul();
    list << li("Element 1") << li("Element 2");
    CHECK(list.str() == "<ul><li>Element 1</li><li>Element 2</li></ul>");
}

TEST_CASE("shouldAllowCommaSeparation", "[HtmlTests]") {
    auto list = ul(li("Element 1"), li("Element 2"));
    CHECK(list.str() == "<ul><li>Element 1</li><li>Element 2</li></ul>");
}

TEST_CASE("shouldHandleAttributes", "[HtmlTests]") {
    auto elem = span("Some text").clazz("class");
    CHECK(elem.str() == "<span class=\"class\">Some text</span>");
}

TEST_CASE("shouldAppendLikeAStreamFromEmpty", "[HtmlTests]") {
    auto elem = empty() << "Text " << 1 << ' ' << 10.23;
    CHECK(elem.str() == "Text 1 10.23");
}

TEST_CASE("shouldAppendLikeAStreamFromEmptyWithSingleInt", "[HtmlTests]") {
    auto elem = empty() << 1;
    CHECK(elem.str() == "1");
}

TEST_CASE("shouldNotNeedExtraMarkupForTextNodes", "[HtmlTests]") {
    auto elem = text("This ") << text("is") << text(" a test") << ".";
    CHECK(elem.str() == "This is a test.");
}

TEST_CASE("shouldWorkWithAnchors", "[HtmlTests]") {
    auto elem = a("http://google.com/?q=badgers", div(span("foo")));
    CHECK(elem.str() == "<a href=\"http://google.com/?q=badgers\"><div><span>foo</span></div></a>");
}

TEST_CASE("shouldAddAll", "[HtmlTests]") {
    std::vector<std::string> strings = {"Hi", "Moo", "Foo"};
    auto list = ul().addAll(strings, [](const std::string& s) { return li(s); });
    CHECK(list.str() == "<ul><li>Hi</li><li>Moo</li><li>Foo</li></ul>");
}
