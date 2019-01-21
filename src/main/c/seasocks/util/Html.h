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

#pragma once

#include "seasocks/ToString.h"

#include <functional>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

namespace seasocks {

namespace html {

class Element {
    bool _isTextNode;
    bool _needsClose;
    std::string _nameOrText;
    std::string _attributes;
    std::list<Element> _children;

    void append() {
    }

    template <typename T, typename... Args>
    void append(const T& t, Args&&... args) {
        operator<<(t);
        append(std::forward<Args>(args)...);
    }

    Element()
            : _isTextNode(true), _needsClose(false) {
    }

public:
    template <typename... Args>
    Element(const std::string& name, bool needsClose, Args&&... args)
            : _isTextNode(false),
              _needsClose(needsClose),
              _nameOrText(name) {
        append(std::forward<Args>(args)...);
    }

    template <typename T>
    static Element textElement(const T& text) {
        Element e;
        e._nameOrText = toString(text);
        return e;
    }

    template <typename T>
    Element& addAttribute(const char* attr, const T& value) {
        _attributes += std::string(" ") + attr + "=\"" + toString(value) + "\"";
        return *this;
    }

    Element& clazz(const std::string& clazz) {
        return addAttribute("class", clazz);
    }

    Element& title(const std::string& title) {
        return addAttribute("title", title);
    }

    Element& style(const std::string& style) {
        return addAttribute("style", style);
    }

    Element& alt(const std::string& alt) {
        return addAttribute("alt", alt);
    }

    Element& hidden() {
        return addAttribute("style", "display:none;");
    }

    Element& id(const std::string& id) {
        return addAttribute("id", id);
    }

    Element& operator<<(const char* child) {
        _children.push_back(textElement(child));
        return *this;
    }

    Element& operator<<(const std::string& child) {
        _children.push_back(textElement(child));
        return *this;
    }

    Element& operator<<(const Element& child) {
        _children.push_back(child);
        return *this;
    }

    template <class T>
    typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, Element&>::type
    operator<<(const T& child) {
        _children.push_back(textElement(child));
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const Element& elem) {
        if (elem._isTextNode) {
            os << elem._nameOrText;
            for (const auto& it : elem._children) {
                os << it;
            }
            return os;
        }
        os << "<" << elem._nameOrText << elem._attributes << ">";
        for (const auto& it : elem._children) {
            os << it;
        }
        if (elem._needsClose) {
            os << "</" << elem._nameOrText << ">";
        }
        return os;
    }

    template <typename C, typename F>
    Element& addAll(const C& container, F functor) {
        for (auto it = container.cbegin(); it != container.cend(); ++it) {
            operator<<(functor(*it));
        }
        return *this;
    }

    std::string str() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }
};

#define HTMLELEM(XX)                                            \
    template <typename... Args>                                 \
    inline Element XX(Args&&... args) {                         \
        return Element(#XX, true, std::forward<Args>(args)...); \
    }
HTMLELEM(html)
HTMLELEM(head)
HTMLELEM(title)
HTMLELEM(body)
HTMLELEM(h1)
HTMLELEM(h2)
HTMLELEM(h3)
HTMLELEM(h4)
HTMLELEM(h5)
HTMLELEM(table)
HTMLELEM(thead)
HTMLELEM(tbody)
HTMLELEM(tr)
HTMLELEM(td)
HTMLELEM(th)
HTMLELEM(div)
HTMLELEM(span)
HTMLELEM(ul)
HTMLELEM(ol)
HTMLELEM(li)
HTMLELEM(label)
HTMLELEM(button)
#undef HTMLELEM

inline Element empty() {
    return Element::textElement("");
}

template <typename T>
inline Element text(const T& t) {
    return Element::textElement(t);
}

inline Element img(const std::string& src) {
    return Element("img", false).addAttribute("src", src);
}

inline Element checkbox() {
    return Element("input", false).addAttribute("type", "checkbox");
}

inline Element externalScript(const std::string& src) {
    return Element("script", true).addAttribute("src", src).addAttribute("type", "text/javascript");
}

inline Element inlineScript(const std::string& script) {
    return Element("script", true, script).addAttribute("type", "text/javascript");
}

inline Element link(const std::string& href, const std::string& rel) {
    return Element("link", false).addAttribute("href", href).addAttribute("rel", rel);
}

template <typename... Args>
inline Element a(const std::string& href, Args&&... args) {
    return Element("a", true, std::forward<Args>(args)...).addAttribute("href", href);
}

} // namespace html

} // namespace seasocks
