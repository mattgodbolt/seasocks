#ifndef _SEASOCKS_PRINTFLOGGER_H_INCLUDED
#define _SEASOCKS_PRINTFLOGGER_H_INCLUDED

#include "Logger.h"
#include <stdio.h>

namespace seasocks {

class PrintfLogger : public Logger {
public:
	PrintfLogger(Level minLevelToLog = Level::DEBUG) : minLevelToLog(minLevelToLog) {
	}

	~PrintfLogger() {
	}

	virtual void log(Level level, const char* message) {
		if (level >= minLevelToLog) {
			printf("%s: %s\n", levelToString(level), message);
		}
	}

	Level minLevelToLog;
};

}  // namespace seasocks

#endif  // _SEASOCKS_PRINTFLOGGER_H_INCLUDED
