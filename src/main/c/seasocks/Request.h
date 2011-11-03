#pragma once

#include <netinet/in.h>
#include <boost/shared_ptr.hpp>
#include "seasocks/credentials.h"

namespace SeaSocks {

class Request {
public:
	/**
	 * Returns the credentials associated with this request.
	 */
	virtual boost::shared_ptr<Credentials> credentials() = 0;

	virtual const sockaddr_in& getRemoteAddress() const = 0;

	virtual const std::string& getRequestUri() const = 0;

};

}  // namespace SeaSocks
