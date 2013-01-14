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
