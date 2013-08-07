#include "seasocks/util/PathHandler.h"

#include "seasocks/Request.h"
#include "seasocks/Response.h"
#include "seasocks/util/CrackedUri.h"

using namespace seasocks;

CrackedUriPageHandler::Ptr PathHandler::add(const CrackedUriPageHandler::Ptr& handler) {
    _handlers.emplace_back(handler);
    return _handlers.back();
}

std::shared_ptr<Response> PathHandler::handle(
        const CrackedUri& uri, const Request& request) {
    if (uri.path()[0] != _path) return Response::unhandled();

    auto shiftedUri = uri.shift();

    for (const auto &it : _handlers) {
        auto response = it->handle(shiftedUri, request);
        if (response != Response::unhandled()) return response;
    }
    return Response::unhandled();
}
