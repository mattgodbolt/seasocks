#ifndef _SEASOCKS_STRINGUTIL_H_
#define _SEASOCKS_STRINGUTIL_H_

#include <netinet/in.h>

namespace SeaSocks {

char* skipWhitespace(char* str);
char* skipNonWhitespace(char* str);
char* shift(char*& str);

const char* getLastError();

const char* formatAddress(const sockaddr_in& address);

}  // namespace SeaSocks

#endif  // _SEASOCKS_STRINGUTIL_H_
