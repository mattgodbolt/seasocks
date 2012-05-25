#pragma once

#include <string>
#include <memory>
#include "seasocks/credentials.h"
#include "seasocks/Response.h"

namespace SeaSocks {

class Request;

class PageHandler {
public:
	virtual ~PageHandler() {}

	virtual std::shared_ptr<Response> handle(const Request& request) = 0;
};

}
