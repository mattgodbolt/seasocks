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

#include "internal/ConcreteResponse.h"

#include "seasocks/Response.h"

namespace seasocks {

std::shared_ptr<Response> Response::unhandled() {
    static std::shared_ptr<Response> unhandled;
    return unhandled;
}

std::shared_ptr<Response> Response::notFound() {
    static std::shared_ptr<Response> notFound = std::make_shared<ConcreteResponse>(
        ResponseCode::NotFound,
        "Not found", "text/plain",
        SynchronousResponse::Headers(), false);
    return notFound;
}

std::shared_ptr<Response> Response::error(ResponseCode code, const std::string& error) {
    return std::make_shared<ConcreteResponse>(
        code, error, "text/plain",
        SynchronousResponse::Headers(), false);
}

std::shared_ptr<Response> Response::textResponse(const std::string& response) {
    return std::make_shared<ConcreteResponse>(
        ResponseCode::Ok,
        response, "text/plain",
        SynchronousResponse::Headers(), true);
}

std::shared_ptr<Response> Response::jsonResponse(const std::string& response) {
    return std::make_shared<ConcreteResponse>(
        ResponseCode::Ok, response,
        "application/json",
        SynchronousResponse::Headers(), true);
}

std::shared_ptr<Response> Response::htmlResponse(const std::string& response) {
    return std::make_shared<ConcreteResponse>(
        ResponseCode::Ok, response,
        "text/html",
        SynchronousResponse::Headers(), true);
}

}
