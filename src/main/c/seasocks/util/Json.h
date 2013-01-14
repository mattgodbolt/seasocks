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
    JsonnedString() {

    }
    JsonnedString(const std::string& s) : std::string(s) {
    }
    JsonnedString(const std::stringstream& str) : std::string(str.str()) {
    }
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

inline void jsonKeyPairToStream(std::ostream& str) {
}

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
