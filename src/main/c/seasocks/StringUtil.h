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

#pragma once

#ifdef _WIN32
#include "../../../win32/winsock_includes.h"
#else
#include <arpa/inet.h>
#endif

#include <ctime>
#include <string>
#include <vector>

namespace seasocks {

char* skipWhitespace(char* str);
char* skipNonWhitespace(char* str);
char* shift(char*& str);
std::string trimWhitespace(const std::string& str);

std::string getLastError();
std::string formatAddress(const sockaddr_in& address);

std::vector<std::string> split(const std::string& input, char splitChar);

void replace(std::string& string, const std::string& find, const std::string& replace);

bool caseInsensitiveSame(const std::string& lhs, const std::string& rhs);

std::string webtime(time_t time);

std::string now();

std::string getWorkingDir();

bool endsWith(const std::string& target, const std::string& endsWithWhat);


} // namespace seasocks
