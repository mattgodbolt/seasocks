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

// Seasocks is single-threaded. For scalability, multiple servers can be created that listen to the
// same port and have the kernel pick one of the listening threads to serve the request.
//
// Note that you will need to be very careful about synchronization throughout your program, so think
// carefully before taking this on. In most cases, the single threaded implementation, or a bunch of them
// behind a load balancer might be a simpler and more reliable solution.
//
// When you run the program and hit it with curl, you will see "id" with different numbers indicating different
// thread that handled your request.

#include "seasocks/PageHandler.h"
#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/WebSocket.h"

#include <utility>
#include <vector>
#include <thread>
#include <sstream>

using namespace seasocks;

class ResponseHandler final: public PageHandler {
public:
    explicit ResponseHandler(size_t id, std::shared_ptr<std::atomic<uint64_t>> totalHits)
        : id_{id}
        , totalHits_{std::move(totalHits)}
    {}

    std::shared_ptr<Response> handle(const Request&) override {
        auto hits = totalHits_->fetch_add(1);
        std::stringstream ss;
        ss << "{id: " << id_ << ", totalHits: " << hits << "}";
        return Response::jsonResponse(ss.str());
    }
private:
    // this state is confined to the handler, no synchronization needed.
    size_t id_;
    // shared mutable state, has to be threadsafe, so we use atomics.
    std::shared_ptr<std::atomic<uint64_t>> totalHits_;
};

int main(int /* argc */, const char */*argv*/[]) {
    auto logger = std::make_shared<PrintfLogger>();
    const size_t NTHREADS = 4;

    // Any state that is shared must be threadsafe and lifetime must be long enough.
    auto hitCounts = std::make_shared<std::atomic<uint64_t>>();
    std::vector<std::thread> servers;
    servers.reserve(NTHREADS);
    for (size_t i = 0; i < NTHREADS; ++i) {
        servers.emplace_back([=]() mutable {
            Server server{std::move(logger)};
            server.addPageHandler(std::make_shared<ResponseHandler>(i, std::move(hitCounts)));
            server.startListening(9000);
            server.loop();
        });
    }

    for (auto &server: servers) {
        server.join();
    }
}
