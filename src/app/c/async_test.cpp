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

// A simple test designed to show how one might use asynchronous responses.

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

struct AsyncResponse : Response {
    Server &_server;
    string _response;
    AsyncResponse(Server &server) : _server(server) {}

    struct RespondOnServerThread : Server::Runnable {
        mutex _mutex;
        ResponseWriter *_writer;
        string _response;
        RespondOnServerThread(ResponseWriter &writer)
        : _writer(&writer) {}
        void setResponse(string response) {
            lock_guard<decltype(_mutex)> lock(_mutex);
            _response = response;
        }
        void cancel() {
            lock_guard<decltype(_mutex)> lock(_mutex);
            _writer = nullptr;
        }
        virtual void run() override {
            lock_guard<decltype(_mutex)> lock(_mutex);  // TODO: lock unnnecesaary? seems like it can't happen any more
            if (_writer) {
                _writer->begin(ResponseCode::Ok);
                _writer->payload(_response.data(), _response.length());
                _writer->finish(false);
            }
        }
    };
    shared_ptr<RespondOnServerThread> _responder;

    virtual void handle(ResponseWriter &writer) override {
        assert(_responder.get() == nullptr);
        // Construct the shared_ptr to responder here. We'll give it to the
        // lambda in the thread so it remains live until the later of the db
        // request being complete, and this response is complete. If while the
        // request is still going, we get a cancelation, we'll tell the response
        // it's canceled.
        _responder = make_shared<RespondOnServerThread>(writer);
        auto &server = _server;
        auto responder = _responder;
        thread t([&server, responder] () mutable {
            usleep(5000000); // A long database query...
            responder->setResponse("some kind of response");
            server.execute(responder); // run our response
        });
        t.detach();
    }
    virtual void cancel() override {
        _responder->cancel();
    }
};

struct MyHandler : CrackedUriPageHandler {
    virtual std::shared_ptr<Response> handle(
            const CrackedUri &/*uri*/, const Request &request) override {
        // TODO: the response can be the runnable here too if we kick off the lambda here
        return make_shared<AsyncResponse>(request.server());
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    shared_ptr<Logger> logger(new PrintfLogger(Logger::DEBUG));

    Server server(logger);
    auto root = make_shared<RootPageHandler>();
    auto pathHandler = make_shared<PathHandler>("data", make_shared<MyHandler>());
    root->add(pathHandler);
    server.addPageHandler(root);

    server.serve("src/ws_test_web", 9090);
    return 0;
}
