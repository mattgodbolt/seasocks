#ifndef SEASOCKS_LOGGER_H_INCLUDED
#define SEASOCKS_LOGGER_H_INCLUDED

namespace SeaSocks {

/**
 * Class to send debug logging information to.
 */
class Logger {
public:
	virtual ~Logger() {}

	enum Level {
		DEBUG,  // NB DEBUG is usually opted-out of at compile-time.
		INFO,
		WARNING,
		ERROR,
		SEVERE,
	};

	virtual void log(Level level, const char* message) = 0;

	void debug(const char* message, ...);
	void info(const char* message, ...);
	void warning(const char* message, ...);
	void error(const char* message, ...);
	void severe(const char* message, ...);

	static const char* levelToString(Level level) {
		switch (level) {
		case DEBUG: return "debug";
		case INFO: return "info";
		case WARNING: return "warning";
		case ERROR: return "ERROR";
		case SEVERE: return "SEVERE";
		default: return "???";
		}
	}
};

}  // namespace SeaSocks

#endif // SEASOCKS_LOGGER_H_INCLUDED
