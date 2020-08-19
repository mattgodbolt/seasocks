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

#include "seasocks/StringUtil.h"

#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <system_error>

namespace seasocks {

char* skipWhitespace(char* str) {
    while (isspace(*str)) {
        ++str;
    }
    return str;
}

char* skipNonWhitespace(char* str) {
    while (*str && !isspace(*str)) {
        ++str;
    }
    return str;
}

char* shift(char*& str) {
    if (str == nullptr) {
        return nullptr;
    }
    char* startOfWord = skipWhitespace(str);
    if (*startOfWord == 0) {
        str = startOfWord;
        return nullptr;
    }
    char* endOfWord = skipNonWhitespace(startOfWord);
    if (*endOfWord != 0) {
        *endOfWord++ = 0;
    }
    str = endOfWord;
    return startOfWord;
}

std::string trimWhitespace(const std::string& str) {
    auto l = [](const auto& c) { return std::isspace(c); };
    const auto start = std::find_if_not(str.begin(), str.end(), l);
    const auto end = std::find_if_not(str.rbegin(), str.rend(), l).base();

    return (start < end ? std::string(start, end) : "");
}

std::string getLastError() {
    const auto error = std::generic_category().default_error_condition(errno);
    return "(" + std::to_string(error.value()) + ") " + error.message();
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
    if (input.empty()) {
        return {};
    }

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

void replace(std::string& string, const std::string& find, const std::string& replace) {
    if (find.empty()) {
        return;
    }

    size_t pos = 0;
    const size_t findLen = find.length();
    const size_t replaceLen = replace.length();
    while ((pos = string.find(find, pos)) != std::string::npos) {
        string = string.substr(0, pos) + replace + string.substr(pos + findLen);
        pos += replaceLen;
    }
}

bool caseInsensitiveSame(const std::string& lhs, const std::string& rhs) {
    return strcasecmp(lhs.c_str(), rhs.c_str()) == 0;
}

std::string webtime(time_t time) {
    struct tm timeValue;
    gmtime_r(&time, &timeValue);
    char buf[1024];
    // Wed, 20 Apr 2011 17:31:28 GMT
    strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %H:%M:%S %Z", &timeValue);
    return buf;
}

std::string now() {
    return webtime(time(nullptr));
}

}
