#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include "seasocks/credentials.h"
#include "seasocks/Response.h"

namespace SeaSocks {

class Request;

class PageHandler {
public:
	virtual ~PageHandler() {}

	virtual boost::shared_ptr<Response> handle(const Request& request) = 0;
};

}
