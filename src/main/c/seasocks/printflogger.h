#ifndef _SEASOCKS_PRINTFLOGGER_H_INCLUDED
#define _SEASOCKS_PRINTFLOGGER_H_INCLUDED

#include "logger.h"
#include <stdio.h>

namespace SeaSocks {

class PrintfLogger : public Logger {
public:
	virtual void log(Level level, const char* message) {
		printf("%s: %s\n", levelToString(level), message);
	}
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_PRINTFLOGGER_H_INCLUDED
