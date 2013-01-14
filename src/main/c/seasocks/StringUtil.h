#ifndef _SEASOCKS_STRINGUTIL_H_
#define _SEASOCKS_STRINGUTIL_H_

#include <netinet/in.h>
#include <vector>
#include <string>

namespace seasocks {

char* skipWhitespace(char* str);
char* skipNonWhitespace(char* str);
char* shift(char*& str);

std::string getLastError();
std::string formatAddress(const sockaddr_in& address);

std::vector<std::string> split(const std::string& input, char splitChar);

void replace(std::string& string, const std::string& find, const std::string& replace);

}  // namespace seasocks

#endif  // _SEASOCKS_STRINGUTIL_H_
