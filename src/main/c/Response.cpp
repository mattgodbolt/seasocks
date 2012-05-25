#include "seasocks/Response.h"

#include "internal/ConcreteResponse.h"

using namespace SeaSocks;

namespace SeaSocks {

std::shared_ptr<Response> Response::unhandled() {
	static std::shared_ptr<Response> unhandled;
	return unhandled;
}

std::shared_ptr<Response> Response::notFound() {
	static std::shared_ptr<Response> notFound(new ConcreteResponse(ResponseCode::NotFound, "Not found", "text/plain", Response::Headers(), false));
	return notFound;
}

std::shared_ptr<Response> Response::error(ResponseCode code, const std::string& reason) {
	return std::shared_ptr<Response>(new ConcreteResponse(code, reason, "text/plain", Response::Headers(), false));
}

std::shared_ptr<Response> Response::textResponse(const std::string& response) {
	return std::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/plain", Response::Headers(), true));
}

std::shared_ptr<Response> Response::jsonResponse(const std::string& response) {
	return std::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "application/json", Response::Headers(), true));
}

std::shared_ptr<Response> Response::htmlResponse(const std::string& response) {
	return std::shared_ptr<Response>(
			new ConcreteResponse(ResponseCode::Ok, response, "text/html", Response::Headers(), true));
}

}
