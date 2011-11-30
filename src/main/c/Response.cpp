#include "seasocks/Response.h"

#include "internal/ConcreteResponse.h"

using namespace SeaSocks;

namespace SeaSocks {

boost::shared_ptr<Response> Response::unhandled() {
	static boost::shared_ptr<Response> unhandled;
	return unhandled;
}

boost::shared_ptr<Response> Response::notFound() {
	static boost::shared_ptr<Response> notFound(new ConcreteResponse(ResponseCode::NotFound, "Not found", "text/plain", Response::Headers(), false));
	return notFound;
}

boost::shared_ptr<Response> Response::error(ResponseCode code, const std::string& reason) {
	return boost::shared_ptr<Response>(new ConcreteResponse(code, reason, "text/plain", Response::Headers(), false));
}

boost::shared_ptr<Response> Response::textResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/plain", Response::Headers(), true));
}

boost::shared_ptr<Response> Response::jsonResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "application/json", Response::Headers(), true));
}

boost::shared_ptr<Response> Response::htmlResponse(const std::string& response) {
	return boost::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/html", Response::Headers(), true));
}

}
