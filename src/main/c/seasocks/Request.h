#pragma once

#include "seasocks/Credentials.h"

#include <netinet/in.h>

#include <cstdint>
#include <memory>

namespace seasocks {

class Request {
public:
	virtual ~Request() {}

	enum Verb {
		Invalid,
		WebSocket,
		Get,
		Put,
		Post,
		Delete,
	};

	virtual Verb verb() const = 0;

    static const char* name(Verb v);

	/**
	 * Returns the credentials associated with this request.
	 */
	virtual std::shared_ptr<Credentials> credentials() const = 0;

	virtual const sockaddr_in& getRemoteAddress() const = 0;

	virtual const std::string& getRequestUri() const = 0;

	virtual size_t contentLength() const = 0;

	virtual const uint8_t* content() const = 0;

	virtual bool hasHeader(const std::string& name) const = 0;

	virtual std::string getHeader(const std::string& name) const = 0;
};

}  // namespace seasocks
