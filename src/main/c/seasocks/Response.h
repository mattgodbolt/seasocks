#pragma once

#include <boost/shared_ptr.hpp>
#include <string>

namespace SeaSocks {

class Response {
public:
	virtual ~Response() {}
	virtual int responseCode() const = 0;

	virtual const char* payload() const = 0;
	virtual size_t payloadSize() const = 0;

	virtual std::string contentType() const = 0;

	static boost::shared_ptr<Response> notFound();
	static boost::shared_ptr<Response> textResponse(const std::string& response);
	static boost::shared_ptr<Response> jsonResponse(const std::string& response);
	static boost::shared_ptr<Response> htmlResponse(const std::string& response);
};

}
