#include "seasocks/util/RootPageHandler.h"

#include "seasocks/Request.h"
#include "seasocks/Response.h"
#include "seasocks/util/CrackedUri.h"

#include <stdexcept>

using namespace seasocks;

RootPageHandler::RootPageHandler() {
}

RootPageHandler::~RootPageHandler() {
}

void RootPageHandler::add(const std::string& url, const std::shared_ptr<CrackedUriPageHandler>& handler) {
    if (_handlers.find(url) != _handlers.end()) {
        throw std::runtime_error("Duplicate page handler added for " + url);
    }
    _handlers.emplace(url, handler);
}

std::shared_ptr<Response> RootPageHandler::handle(const Request& request) {
    CrackedUri uri(request.getRequestUri());
    auto it = _handlers.find(uri.path()[0]);
    if (it != _handlers.end()) {
        return it->second->handle(uri, request);
    }
    return Response::unhandled();
}
