#ifndef _SEASOCKS_JSON_H_
#define _SEASOCKS_JSON_H_

#include <string>
#include <sstream>

namespace SeaSocks {

struct EpochTimeAsLocal {
	time_t t;
	EpochTimeAsLocal(time_t t) : t(t) {}
};

inline void jsonToStream(std::ostream& str) {}
void jsonToStream(std::ostream& str, const char* t);
void jsonToStream(std::ostream& str, const EpochTimeAsLocal& t);
inline void jsonToStream(std::ostream& str, const std::string& t) {
	jsonToStream(str, t.c_str());
}
template<typename T>
void jsonToStream(std::ostream& str, const T& t) {
	str << t;
}
template<typename T, typename ... Args>
void jsonToStream(std::ostream& str, const T& t, Args... args) {
	jsonToStream(str, t);
	str << ", ";
	jsonToStream(str, args...);
}

inline void jsonKeyPairToStream(std::ostream& str) {
}

template<typename T>
void jsonKeyPairToStream(std::ostream& str, const char* key, const T& value) {
	jsonToStream(str, key);
	str << ": ";
	jsonToStream(str, value);
}
template<typename T>
void jsonKeyPairToStream(std::ostream& str, const std::string& key, const T& value) {
	jsonKeyPairToStream(str, key.c_str(), value);
}
template<typename K, typename V, typename... Args>
void jsonKeyPairToStream(std::ostream& str, const K& key, const V& value, Args... args) {
	jsonKeyPairToStream(str, key, value);
	str << ", ";
	jsonKeyPairToStream(str, args...);
}

///////////////////////////////////

template<typename... Args>
std::string makeMap(Args...args) {
	std::stringstream str;
	str << '{';
	jsonKeyPairToStream(str, args...);
	str << '}';
	return str.str();
}

template<typename ... Args>
std::string makeArray(Args... args) {
	std::stringstream str;
	str << '[';
	jsonToStream(str, args...);
	str << ']';
	return str.str();
}

template<typename ... Args>
std::string makeExecString(const char* function, Args... args) {
	std::stringstream str;
	str << function << "(";
	jsonToStream(str, args...);
	str << ")";
	return str.str();
}

}  // namespace SeaSocks

#endif
