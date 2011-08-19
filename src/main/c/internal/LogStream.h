#ifndef LOGSTREAM_H_
#define LOGSTREAM_H_

#include "internal/Debug.h"

// Internal stream helpers for logging.
#include <sstream>

#define LS_LOG(LOG, LEVEL, STUFF) \
{ \
	std::ostringstream o; \
	o << STUFF; \
	(LOG)->log(Logger::LEVEL, o.str().c_str()); \
}

#define LS_DEBUG(LOG, STUFF) 	LS_LOG(LOG, DEBUG, STUFF)
#define LS_INFO(LOG, STUFF) 	LS_LOG(LOG, INFO, STUFF)
#define LS_WARNING(LOG, STUFF) 	LS_LOG(LOG, WARNING, STUFF)
#define LS_ERROR(LOG, STUFF) 	LS_LOG(LOG, ERROR, STUFF)
#define LS_SEVERE(LOG, STUFF) 	LS_LOG(LOG, SEVERE, STUFF)

#endif /* LOGSTREAM_H_ */
