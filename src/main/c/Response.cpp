#include "seasocks/Response.h"

using namespace SeaSocks;

namespace {

class ConcreteResponse : public Response {
	int _responseCode;
	const std::string _payload;
	const std::string _contentType;
public:
	ConcreteResponse(int responseCode, const std::string& payload, const std::string& contentType) :
		_responseCode(responseCode), _payload(payload), _contentType(contentType) {}

	virtual int responseCode() const {
		return _responseCode;
	}

	virtual const char* payload() const {
		return _payload.c_str();
	}

	virtual size_t payloadSize() const {
		return _payload.size();
	}

	virtual std::string contentType() const {
		return _contentType;
	}
};

}

namespace SeaSocks {

boost::shared_ptr<Response> Response::notFound() {
	static boost::shared_ptr<Response> notFound(new ConcreteResponse(404, "Not found", ""));
	return notFound;
}

boost::shared_ptr<Response> Response::textResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(200, response, "text/plain"));
}

boost::shared_ptr<Response> Response::htmlResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(200, response, "text/html"));
}

}
