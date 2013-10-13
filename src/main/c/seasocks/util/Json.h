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

class JsonnedString : public std::string {
public:
    JsonnedString() {}
    JsonnedString(const std::string& s) : std::string(s) {}
    JsonnedString(const std::stringstream& str) : std::string(str.str()) {}
};

inline void jsonToStream(std::ostream& str) {}

inline void jsonToStream(std::ostream& str, const JsonnedString& done) {
    str << done;
}
void jsonToStream(std::ostream& str, const char* t);

void jsonToStream(std::ostream& str, bool b);

struct EpochTimeAsLocal { time_t t; EpochTimeAsLocal(time_t t) : t(t) {} };
void jsonToStream(std::ostream& str, const EpochTimeAsLocal& t);

inline void jsonToStream(std::ostream& str, const std::string& t) {
    jsonToStream(str, t.c_str());
}

template<typename T>
typename std::enable_if<std::is_fundamental<T>::value, void>::type
jsonToStream(std::ostream& str, const T& t) {
    str << t;
}

template<typename T>
typename std::enable_if<!std::is_fundamental<T>::value, void>::type
jsonToStream(std::ostream& str, const T& t) {
    str << '"' << t << '"';
}

template<typename T, typename ... Args>
void jsonToStream(std::ostream& str, const T& t, Args... args) {
    jsonToStream(str, t);
    str << ",";
    jsonToStream(str, args...);
}

inline void jsonKeyPairToStream(std::ostream& str) {}

template<typename T>
void jsonKeyPairToStream(std::ostream& str, const char* key, const T& value) {
    jsonToStream(str, key);
    str << ":";
    jsonToStream(str, value);
}

template<typename T>
void jsonKeyPairToStream(std::ostream& str, const std::string& key, const T& value) {
    jsonKeyPairToStream(str, key.c_str(), value);
}

template<typename K, typename V, typename... Args>
void jsonKeyPairToStream(std::ostream& str, const K& key, const V& value, Args... args) {
    jsonKeyPairToStream(str, key, value);
    str << ",";
    jsonKeyPairToStream(str, args...);
}

///////////////////////////////////

template<typename... Args>
JsonnedString makeMap(Args...args) {
    std::stringstream str;
    str << '{';
    jsonKeyPairToStream(str, args...);
    str << '}';
    return JsonnedString(str);
}

template<typename K, typename V>
JsonnedString makeMap(const std::map<K, V>& m) {
    std::stringstream str;
    str << "{";
    bool first = true;
    std::for_each(m.begin(), m.end(), [&] (decltype(*m.begin()) it) {
        if (!first) str << ",";
        first = false;
        jsonKeyPairToStream(str, it.first, it.second);
    });
    str << "}";
    return JsonnedString(str);
}

template<typename ... Args>
JsonnedString makeArray(Args... args) {
    std::stringstream str;
    str << '[';
    jsonToStream(str, args...);
    str << ']';
    return JsonnedString(str);
}

template<typename T>
seasocks::JsonnedString makeArray(const T &list) {
    std::stringstream str;
    str << '[';
    bool first = true;
    for (const auto &s : list) {
        if (!first) {
            str << ',';
        }
        first = false;
        jsonToStream(str, s);
    };
    str << ']';
    return seasocks::JsonnedString(str);
}

template<typename ... Args>
JsonnedString makeExecString(const char* function, Args... args) {
    std::stringstream str;
    str << function << '(';
    jsonToStream(str, args...);
    str << ')';
    return JsonnedString(str);
}

inline JsonnedString makeArray(const std::vector<JsonnedString>& list) {
    std::stringstream str;
    str << '[';
    bool first = true;
    std::for_each(list.begin(), list.end(), [&] (const JsonnedString& s) {
        if (!first) {
            str << ',';
        }
        first = false;
        str << s;
    });
    str << ']';
    return JsonnedString(str);
}

}
