#pragma once

#include <seasocks/ResponseCode.h>

#include <boost/shared_ptr.hpp>
#include <string>

namespace SeaSocks {

class Response {
public:
	virtual ~Response() {}
	virtual ResponseCode responseCode() const = 0;

	virtual const char* payload() const = 0;
	virtual size_t payloadSize() const = 0;

	virtual std::string contentType() const = 0;

	static boost::shared_ptr<Response> unhandled();

	static boost::shared_ptr<Response> notFound();

	static boost::shared_ptr<Response> error(ResponseCode code, const std::string& error);

	static boost::shared_ptr<Response> textResponse(const std::string& response);
	static boost::shared_ptr<Response> jsonResponse(const std::string& response);
	static boost::shared_ptr<Response> htmlResponse(const std::string& response);
};

}
