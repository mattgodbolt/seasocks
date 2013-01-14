#ifndef _SEASOCKS_IGNORINGLOGGER_H_INCLUDED
#define _SEASOCKS_IGNORINGLOGGER_H_INCLUDED

#include "Logger.h"

namespace SeaSocks {

class IgnoringLogger : public Logger {
public:
	virtual void log(Level level, const char* message) {
	}
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_IGNORINGLOGGER_H_INCLUDED
