#pragma once

#include "seasocks/Logger.h"

namespace seasocks {

class IgnoringLogger : public Logger {
public:
    virtual void log(Level level, const char* message) {
    }
};

}  // namespace seasocks
