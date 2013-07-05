#include "seasocks/util/RootPageHandler.h"

#include "seasocks/Request.h"
#include "seasocks/Response.h"
#include "seasocks/util/CrackedUri.h"

using namespace seasocks;

RootPageHandler::RootPageHandler() {
}

RootPageHandler::~RootPageHandler() {
}

void RootPageHandler::add(const std::shared_ptr<CrackedUriPageHandler>& handler) {
    _handlers.emplace_back(handler);
}

std::shared_ptr<Response> RootPageHandler::handle(const Request& request) {
    CrackedUri uri(request.getRequestUri());
    for (const auto &it : _handlers) {
        auto response = it->handle(uri, request);
        if (response != Response::unhandled()) return response;
    }
    return Response::unhandled();
}
