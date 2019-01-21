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

#include <algorithm>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace seasocks {

///////////////////////////////////

inline void jsonToStream(std::ostream& /*str*/) {
}

void jsonToStream(std::ostream& str, const char* t);

void jsonToStream(std::ostream& str, bool b);

inline void jsonToStream(std::ostream& str, const std::string& t) {
    jsonToStream(str, t.c_str());
}

template <typename O>
class is_jsonable {
    template <typename OO>
    static auto test(int)
        -> decltype(&OO::jsonToStream, std::true_type());

    template <typename>
    static auto test(...) -> std::false_type;

public:
    static constexpr bool value = decltype(test<O>(0))::value;
};

template <typename T>
class is_streamable {
    template <typename TT>
    static auto test(int)
        -> decltype(std::declval<std::ostream&>() << std::declval<TT>(), std::true_type());

    template <typename>
    static auto test(...) -> std::false_type;

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T>
typename std::enable_if<std::is_fundamental<T>::value, void>::type
jsonToStream(std::ostream& str, const T& t) {
    str << t;
}

template <typename T>
typename std::enable_if<is_jsonable<T>::value, void>::type
jsonToStream(std::ostream& str, const T& t) {
    t.jsonToStream(str);
}

template <typename T>
typename std::enable_if<
    !std::is_fundamental<T>::value && is_streamable<T>::value && !is_jsonable<T>::value, void>::type
jsonToStream(std::ostream& str, const T& t) {
    str << '"' << t << '"';
}

template <typename T, typename... Args>
void jsonToStream(std::ostream& str, const T& t, Args&&... args) {
    static_assert(sizeof...(Args) > 0, "Cannot stream an object with no jsonToStream or operator<< method.");
    jsonToStream(str, t);
    str << ",";
    jsonToStream(str, std::forward<Args>(args)...);
}

///////////////////////////////////

inline void jsonKeyPairToStream(std::ostream& /*str*/) {
}

template <typename T>
void jsonKeyPairToStream(std::ostream& str, const char* key, const T& value) {
    jsonToStream(str, key);
    str << ":";
    jsonToStream(str, value);
}

template <typename T>
void jsonKeyPairToStream(std::ostream& str, const std::string& key, const T& value) {
    jsonKeyPairToStream(str, key.c_str(), value);
}

template <typename T>
void jsonKeyPairToStream(std::ostream&, const T&) {
    static_assert(!std::is_same<T, T>::value, // To make the assertion depend on T
                  "Requires an even number of parameters. If you're trying to build a map from an existing std::map or similar, use makeMapFromContainer");
}

template <typename K, typename V, typename... Args>
void jsonKeyPairToStream(std::ostream& str, const K& key, const V& value, Args&&... args) {
    jsonKeyPairToStream(str, key, value);
    str << ",";
    jsonKeyPairToStream(str, std::forward<Args>(args)...);
}

struct JsonnedString : std::string {
    JsonnedString() = default;
    JsonnedString(const std::string& s)
            : std::string(s) {
    }
    JsonnedString(const std::stringstream& str)
            : std::string(str.str()) {
    }
    void jsonToStream(std::ostream& o) const {
        o << *this;
    }
};
static_assert(is_streamable<JsonnedString>::value, "Internal stream problem");
static_assert(is_jsonable<JsonnedString>::value, "Internal stream problem");

struct EpochTimeAsLocal {
    time_t t;
    EpochTimeAsLocal(time_t time)
            : t(time) {
    }
    void jsonToStream(std::ostream& o) const;
};
static_assert(is_jsonable<EpochTimeAsLocal>::value, "Internal stream problem");

template <typename... Args>
JsonnedString makeMap(Args&&... args) {
    std::stringstream str;
    str << '{';
    jsonKeyPairToStream(str, std::forward<Args>(args)...);
    str << '}';
    return JsonnedString(str);
}

template <typename T>
JsonnedString makeMapFromContainer(const T& m) {
    std::stringstream str;
    str << "{";
    bool first = true;
    for (const auto& it : m) {
        if (!first)
            str << ",";
        first = false;
        jsonKeyPairToStream(str, it.first, it.second);
    }
    str << "}";
    return JsonnedString(str);
}

template <typename... Args>
JsonnedString makeArray(Args&&... args) {
    std::stringstream str;
    str << '[';
    jsonToStream(str, std::forward<Args>(args)...);
    str << ']';
    return JsonnedString(str);
}

template <typename T>
JsonnedString makeArrayFromContainer(const T& list) {
    std::stringstream str;
    str << '[';
    bool first = true;
    for (const auto& s : list) {
        if (!first) {
            str << ',';
        }
        first = false;
        jsonToStream(str, s);
    };
    str << ']';
    return JsonnedString(str);
}

template <typename T>
JsonnedString makeArray(const std::initializer_list<T>& list) {
    std::stringstream str;
    str << '[';
    bool first = true;
    for (const auto& s : list) {
        if (!first) {
            str << ',';
        }
        first = false;
        jsonToStream(str, s);
    };
    str << ']';
    return JsonnedString(str);
}

template <typename... Args>
JsonnedString makeExecString(const char* function, Args&&... args) {
    std::stringstream str;
    str << function << '(';
    jsonToStream(str, std::forward<Args>(args)...);
    str << ')';
    return JsonnedString(str);
}

template <typename T>
JsonnedString to_json(const T& obj) {
    std::stringstream str;
    jsonToStream(str, obj);
    return str.str();
}

}
