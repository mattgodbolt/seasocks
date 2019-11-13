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

// An example to show how one might use asynchronous responses.

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/Response.h"
#include "seasocks/ResponseWriter.h"
#include "seasocks/ResponseCode.h"
#include "seasocks/util/RootPageHandler.h"
#include "seasocks/util/PathHandler.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

using namespace seasocks;

// The AsyncResponse does some long-lived "work" (in this case a big sleep...)
// before responding to the ResponseWriter, in chunks. It uses a new thread to
// perform this "work". As responses can be canceled before the work is
// complete, we must ensure the ResponseWriter used to communicate the response
// is kept alive long enough by holding its shared_ptr in the "work" thread.
// Seasocks will tell the response it has been cancelled (if the connection
// associated with the request is closed); but the ResponseWriter is safe in the
// presence of a closed connection so for simplicity this example does nothing
// in the cancel() method. It is assumed the lifetime of the Server object is
// long enough for all requests to complete before it is destroyed.
struct AsyncResponse : Response {
    Server& _server;
    explicit AsyncResponse(Server& server)
            : _server(server) {
    }

    // From Response:
    void handle(std::shared_ptr<ResponseWriter> writer) override {
        auto& server = _server;
        std::thread t([&server, writer]() mutable {
            using namespace std::literals::chrono_literals;

            std::this_thread::sleep_for(1s); // A long database query...
            std::string response = "some kind of response...beginning<br>";
            server.execute([response, writer] {
                writer->begin(ResponseCode::Ok, TransferEncoding::Chunked);
                writer->header("Content-type", "application/html");
                writer->payload(response.data(), response.length());
            });
            response = "more data...<br>";
            for (auto i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(1s); // more data
                server.execute([response, writer] {
                    writer->payload(response.data(), response.length());
                });
            }
            response = "Done!";
            std::this_thread::sleep_for(100ms); // final data
            server.execute([response, writer] {
                writer->payload(response.data(), response.length());
                writer->finish(true);
            });
        });
        t.detach();
    }
    void cancel() override {
        // If we could cancel the thread, we would do so here. There's no need
        // to invalidate the _writer; any writes to it after this will be
        // silently dropped.
    }
};

struct DataHandler : CrackedUriPageHandler {
    virtual std::shared_ptr<Response> handle(
        const CrackedUri& /*uri*/, const Request& request) override {
        return std::make_shared<AsyncResponse>(request.server());
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    auto logger = std::make_shared<PrintfLogger>(Logger::Level::Debug);

    Server server(logger);
    auto root = std::make_shared<RootPageHandler>();
    auto pathHandler = std::make_shared<PathHandler>("data", std::make_shared<DataHandler>());
    root->add(pathHandler);
    server.addPageHandler(root);

    server.serve("src/async_test_web", 9090);
    return 0;
}
