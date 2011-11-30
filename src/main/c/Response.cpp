#include "seasocks/Response.h"

using namespace SeaSocks;

namespace {

class ConcreteResponse : public Response {
	ResponseCode _responseCode;
	const std::string _payload;
	const std::string _contentType;
public:
	ConcreteResponse(ResponseCode responseCode, const std::string& payload, const std::string& contentType) :
		_responseCode(responseCode), _payload(payload), _contentType(contentType) {}

	virtual ResponseCode responseCode() const {
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

boost::shared_ptr<Response> Response::unhandled() {
	static boost::shared_ptr<Response> unhandled;
	return unhandled;
}

boost::shared_ptr<Response> Response::notFound() {
	static boost::shared_ptr<Response> notFound(new ConcreteResponse(ResponseCode::NotFound, "Not found", ""));
	return notFound;
}

boost::shared_ptr<Response> Response::error(ResponseCode code, const std::string& reason) {
	return boost::shared_ptr<Response>(new ConcreteResponse(code, reason, ""));
}

boost::shared_ptr<Response> Response::textResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/plain"));
}

boost::shared_ptr<Response> Response::jsonResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "application/json"));
}

boost::shared_ptr<Response> Response::htmlResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/html"));
}

}
