#include "seasocks/stringutil.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace SeaSocks {

char* skipWhitespace(char* str) {
	while (isspace(*str)) ++str;
	return str;
}

char* skipNonWhitespace(char* str) {
	while (*str && !isspace(*str)) {
		++str;
	}
	return str;
}

char* shift(char*& str) {
	if (str == NULL) {
		return NULL;
	}
	char* startOfWord = skipWhitespace(str);
	if (*startOfWord == 0) {
		str = startOfWord;
		return NULL;
	}
	char* endOfWord = skipNonWhitespace(startOfWord);
	if (*endOfWord != 0) {
		*endOfWord++ = 0;
	}
	str = endOfWord;
	return startOfWord;
}

std::string getLastError(){
	char errbuf[1024];
	return strerror_r(errno, errbuf, sizeof(errbuf));
}

std::string formatAddress(const sockaddr_in& address) {
	char ipBuffer[24];
	sprintf(ipBuffer,
			"%d.%d.%d.%d:%d",
			(address.sin_addr.s_addr >> 0) & 0xff,
			(address.sin_addr.s_addr >> 8) & 0xff,
			(address.sin_addr.s_addr >> 16) & 0xff,
			(address.sin_addr.s_addr >> 24) & 0xff,
			htons(address.sin_port));
	return ipBuffer;
}

std::vector<std::string> split(const std::string& input, char splitChar) {
	std::vector<std::string> result;
	size_t pos = 0;
	size_t newPos;
	while ((newPos = input.find(splitChar, pos)) != std::string::npos) {
		result.push_back(input.substr(pos, newPos - pos));
		pos = newPos + 1;
	}
	result.push_back(input.substr(pos));
	return result;
}

}
