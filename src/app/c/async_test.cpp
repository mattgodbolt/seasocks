// Copyright (c) 2013-2016, Matt Godbolt
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
#include <unistd.h>

using namespace seasocks;
using namespace std;

// The AsyncResponse does some long-lived "work" (in this case a big sleep...)
// before responding to the ResponseWriter. It uses a new thread to perform this
// "work". As responses can be canceled before the work is complete, we must
// ensure we can handle cancelations coming from the main thread while the work
// is running. Additionally we must ensure the AsyncResponse object lives longer
// than both the request that's being handled and the "work" thread, even if the
// request terminates earlier because it's canceled (e.g. the user closes the
// web browser tab). To achieve this, the thread keeps a copy of the shared_ptr
// to the AsyncResponse. This requires enable_shared_from_this.
struct AsyncResponse : Response, Server::Runnable, enable_shared_from_this<AsyncResponse> {
    Server &_server;
    mutex _mutex;
    ResponseWriter *_writer;
    string _response;
    AsyncResponse(Server &server) : _server(server), _writer(nullptr) {}

    // From Response
    virtual void handle(ResponseWriter &writer) override {
        _writer = &writer;
        auto &server = _server;
        auto responder = shared_from_this(); // pass another copy of the shared_ptr to us into the thread
        thread t([&server, responder] () mutable {
            usleep(5000000); // A long database query...
            responder->setResponse("some kind of response");
        });
        t.detach();
    }
    // On cancelation we must prevent any writes to the _writer; it is no longer
    // valid.
    virtual void cancel() override {
        lock_guard<decltype(_mutex)> lock(_mutex);
        _writer = nullptr;
    }

    // setResponse is called from the long-running thread.
    void setResponse(string response) {
        lock_guard<decltype(_mutex)> lock(_mutex);
        _response = response;
        _server.execute(shared_from_this()); // run our response
        // NB we could avoid running on the server if we've been canceled by
        // now, but we would still need the check in run() anyway; so we don't
        // bother here.
    }
    // From Server::Runnable - this code runs back on the Seasocks thread once
    // we're done. Note we may have been canceled by now.
    virtual void run() override {
        lock_guard<decltype(_mutex)> lock(_mutex);
        if (!_writer) return;  // we were canceled before we got to run
        _writer->begin(ResponseCode::Ok);
        _writer->payload(_response.data(), _response.length());
        _writer->finish(false);
    }

};

struct DataHandler : CrackedUriPageHandler {
    virtual std::shared_ptr<Response> handle(
            const CrackedUri &/*uri*/, const Request &request) override {
        return make_shared<AsyncResponse>(request.server());
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    shared_ptr<Logger> logger(new PrintfLogger(Logger::DEBUG));

    Server server(logger);
    auto root = make_shared<RootPageHandler>();
    auto pathHandler = make_shared<PathHandler>("data", make_shared<DataHandler>());
    root->add(pathHandler);
    server.addPageHandler(root);

    server.serve("src/async_test_web", 9090);
    return 0;
}
