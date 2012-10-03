#pragma once

#include <sstream>
#include <string>

namespace SeaSocks {

template<typename T>
std::string toString(const T& obj) {
	std::stringstream str;
	str << obj;
	return str.str();
}

}
