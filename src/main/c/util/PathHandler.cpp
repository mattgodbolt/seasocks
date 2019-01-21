// Copyright (c) 2013-2017, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
    const auto& path = uri.path();
    if (path.empty() || path[0] != _path) {
        return Response::unhandled();
    }

    auto shiftedUri = uri.shift();

    for (const auto& it : _handlers) {
        auto response = it->handle(shiftedUri, request);
        if (response != Response::unhandled()) {
            return response;
        }
    }
    return Response::unhandled();
}
