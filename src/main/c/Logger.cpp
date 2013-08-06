#include "internal/Debug.h"

#include "seasocks/Logger.h"

#include <stdarg.h>
#include <stdio.h>

namespace seasocks {

const int MAX_MESSAGE_LENGTH = 1024;

#define PRINT_TO_MESSAGEBUF() \
    char messageBuf[MAX_MESSAGE_LENGTH]; \
    va_list args; \
    va_start(args, message); \
    vsnprintf(messageBuf, MAX_MESSAGE_LENGTH, message, args); \
    va_end(args)

void Logger::debug(const char* message, ...) {
#ifdef LOG_DEBUG_INFO
    PRINT_TO_MESSAGEBUF();
    log(DEBUG, messageBuf);
#endif
}

void Logger::access(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(ACCESS, messageBuf);
}

void Logger::info(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(INFO, messageBuf);
}

void Logger::warning(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(WARNING, messageBuf);
}

void Logger::error(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(ERROR, messageBuf);
}

void Logger::severe(const char* message, ...) {
    PRINT_TO_MESSAGEBUF();
    log(SEVERE, messageBuf);
}

}  // namespace seasocks
