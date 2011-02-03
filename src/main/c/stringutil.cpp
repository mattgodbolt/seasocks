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

const char* getLastError() {
	static char errbuf[128];
	return strerror_r(errno, errbuf, sizeof(errbuf));
}

const char* formatAddress(const sockaddr_in& address) {
	static char ipBuffer[24];
	sprintf(ipBuffer,
			"%d.%d.%d.%d:%d",
			(address.sin_addr.s_addr >> 0) & 0xff,
			(address.sin_addr.s_addr >> 8) & 0xff,
			(address.sin_addr.s_addr >> 16) & 0xff,
			(address.sin_addr.s_addr >> 24) & 0xff,
			htons(address.sin_port));
	return ipBuffer;
}

}
