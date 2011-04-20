#ifndef _SEASOCKS_STRINGUTIL_H_
#define _SEASOCKS_STRINGUTIL_H_

#include <netinet/in.h>
#include <vector>
#include <string>

namespace SeaSocks {

char* skipWhitespace(char* str);
char* skipNonWhitespace(char* str);
char* shift(char*& str);

std::string getLastError();
std::string formatAddress(const sockaddr_in& address);

std::vector<std::string> split(const std::string& input, char splitChar);

}  // namespace SeaSocks

#endif  // _SEASOCKS_STRINGUTIL_H_
