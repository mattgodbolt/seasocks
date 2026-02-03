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

#include <experimental/source_location>
#include <memory>
#include <sstream>

namespace seasocks {

/**
 * Class to send debug logging information to.
 */
class Logger {
public:
    virtual ~Logger() = default;

    enum class Level {
        Debug,  // NB Debug is usually opted-out of at compile-time.
        Access, // Used to log page requests etc
        Info,
        Warning,
        Error,
        Severe,
    };

    virtual void log(Level level, const char* message) = 0;

    static const char* levelToString(Level level) {
        switch (level) {
            case Level::Debug:
                return "debug";
            case Level::Access:
                return "access";
            case Level::Info:
                return "info";
            case Level::Warning:
                return "warning";
            case Level::Error:
                return "ERROR";
            case Level::Severe:
                return "SEVERE";
            default:
                return "???";
        }
    }
};

} // namespace seasocks

namespace LS {

namespace detail {
using source_location = std::experimental::source_location;

template <typename>
inline constexpr bool logger_ptr_type = false;

template <>
inline constexpr bool logger_ptr_type<std::shared_ptr<seasocks::Logger>> = true;

template <>
inline constexpr bool logger_ptr_type<seasocks::Logger*> = true;

inline constexpr std::string_view sloc_fname(const source_location& loc) {
    std::string_view sv{loc.file_name()};
    if (auto p = sv.rfind('/'); p != sv.npos)
        sv.remove_prefix(p + 1);
    return sv;
}
}

template <typename LoggerPtr, typename... Args>
struct LOG {
    explicit LOG(LoggerPtr logger, seasocks::Logger::Level level, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(level, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct DEBUG {
    explicit DEBUG(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Debug, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct ACCESS {
    explicit ACCESS(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Access, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct INFO {
    explicit INFO(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Info, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct WARNING {
    explicit WARNING(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Warning, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct ERROR {
    explicit ERROR(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Error, os_.str().c_str());
    }
};

template <typename LoggerPtr, typename... Args>
struct SEVERE {
    explicit SEVERE(LoggerPtr logger, Args&&... args, const detail::source_location& loc = detail::source_location::current()) {
        static_assert(detail::logger_ptr_type<LoggerPtr>);
        std::ostringstream os_;
        os_ << '[' << detail::sloc_fname(loc) << ':' << loc.line() << "] ";
        (os_ << ... << std::forward<Args>(args));
        logger->log(seasocks::Logger::Level::Severe, os_.str().c_str());
    }
};


// Deduction guides
template <typename LoggerPtr, typename... Args>
LOG(LoggerPtr, seasocks::Logger::Level, Args&&...) -> LOG<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
DEBUG(LoggerPtr, Args&&...) -> DEBUG<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
ACCESS(LoggerPtr, Args&&...) -> ACCESS<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
INFO(LoggerPtr, Args&&...) -> INFO<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
WARNING(LoggerPtr, Args&&...) -> WARNING<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
ERROR(LoggerPtr, Args&&...) -> ERROR<LoggerPtr, Args...>;

template <typename LoggerPtr, typename... Args>
SEVERE(LoggerPtr, Args&&...) -> SEVERE<LoggerPtr, Args...>;
}
