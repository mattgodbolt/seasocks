// Copyright (c) 2013-2017, Martin Charles
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

#include "seasocks/PageHandler.h"
#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/SimpleResponse.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

using namespace seasocks;

size_t streamlen(std::shared_ptr<std::istream> stream) {
    stream->seekg(0, stream->end);
    size_t len = stream->tellg();
    stream->seekg(0, stream->beg);
    return len;
}

class MyPageHandler : public PageHandler {
public:
    virtual std::shared_ptr<Response> handle(const Request& request) override {
        if (request.verb() != Request::Verb::Get) {
            return Response::unhandled();
        }

        auto ss = std::make_shared<std::stringstream>();
        (*ss) << "<html><head><title>seasocks example</title></head>"
                 "<body>Hello, "
              << request.credentials()->attributes["fullName"] << "! You asked for " << request.getRequestUri()
              << " and your ip is " << request.getRemoteAddress().sin_addr.s_addr
              << " and a random number is " << rand()
              << "</body></html>";

        auto headers = SimpleResponse::Headers({
            {"Content-Type", "text/html"},
            {"Content-Length", std::to_string(streamlen(ss))},
            {"Connection", "keep-alive"},
        });

        const auto response = std::make_shared<SimpleResponse>(
            ResponseCode::Ok, // response code
            ss,               // stream
            headers,          // headers
            true,             // keep alive
            true              // flush instantly
        );
        return response;
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    auto logger = std::make_shared<PrintfLogger>(Logger::Level::Info);

    Server server(logger);
    server.addPageHandler(std::make_shared<MyPageHandler>());

    server.serve("", 9090);
    return 0;
}
