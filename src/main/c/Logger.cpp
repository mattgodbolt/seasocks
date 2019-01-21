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

#include "internal/Debug.h"

#include "seasocks/Logger.h"

#include <cstdarg>
#include <cstdio>

namespace seasocks {

constexpr int MAX_MESSAGE_LENGTH = 1024;

#define PRINT_TO_MESSAGEBUF()                                 \
    char messageBuf[MAX_MESSAGE_LENGTH];                      \
    va_list args;                                             \
    va_start(args, message);                                  \
    vsnprintf(messageBuf, MAX_MESSAGE_LENGTH, message, args); \
    va_end(args)

void Logger::debug(const char* message, ...) {
#ifdef LOG_DEBUG_INFO
    PRINT_TO_MESSAGEBUF();
    log(Level::Debug, messageBuf);
#else
    (void) message;
#endif
}

void Logger::access(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(Level::Access, messageBuf);
}

void Logger::info(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(Level::Info, messageBuf);
}

void Logger::warning(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(Level::Warning, messageBuf);
}

void Logger::error(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(Level::Error, messageBuf);
}

void Logger::severe(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(Level::Severe, messageBuf);
}

} // namespace seasocks
