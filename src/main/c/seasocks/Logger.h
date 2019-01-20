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

#undef DEBUG    // DEBUG was defined with a value, this is confusing the compiler as the tokenization happened already, need to undefine it so that the DEBUG can be an enumerator

namespace seasocks {

/**
 * Class to send debug logging information to.
 */
class Logger {
public:
    virtual ~Logger() = default;

    enum class Level {
        DEBUG,  // NB DEBUG is usually opted-out of at compile-time.
        ACCESS, // Used to log page requests etc
        INFO,
        WARNING,
        ERROR,
        SEVERE,
    };

    virtual void log(Level level, const char* message) = 0;

    void debug(const char* message, ...);
    void access(const char* message, ...);
    void info(const char* message, ...);
    void warning(const char* message, ...);
    void error(const char* message, ...);
    void severe(const char* message, ...);

    static const char* levelToString(Level level) {
        switch (level) {
        case Level::DEBUG: return "debug";
        case Level::ACCESS: return "access";
        case Level::INFO: return "info";
        case Level::WARNING: return "warning";
        case Level::ERROR: return "ERROR";
        case Level::SEVERE: return "SEVERE";
        default: return "???";
        }
    }
};

}  // namespace seasocks
