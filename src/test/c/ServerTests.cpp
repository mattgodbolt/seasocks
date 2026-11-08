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

#include "seasocks/Server.h"
#include "seasocks/Connection.h"
#include "seasocks/IgnoringLogger.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <thread>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif
#include <chrono>

using namespace seasocks;


TEST_CASE("Server tests", "[ServerTests]") {
    auto logger = std::make_shared<IgnoringLogger>();
    Server server(logger);
    REQUIRE(server.startListening(0));
    std::thread seasocksThread([&] {
        REQUIRE(server.loop());
    });

    std::atomic<int> test(0);
    SECTION("execute should work") {
        server.execute([&] {
            CHECK(test == 0);
            test++;
        });
        for (int i = 0; i < 1000 * 1000 * 1000; ++i) {
            if (test)
                break;
        }
        CHECK(test == 1);
    }

    SECTION("many executes") {
        using namespace std::literals::chrono_literals;

        std::atomic<bool> latch(false);
        for (auto i = 0; i < 100; ++i) {
            for (auto j = 0; j < 100; ++j) {
                server.execute([&] { test++; });
            }
            std::this_thread::sleep_for(10us);
        }
        server.execute([&] { latch = true; });
        for (int i = 0; i < 1000; ++i) {
            std::this_thread::sleep_for(1ms);
            if (latch)
                break;
        }
        CHECK(latch.load() == true);
        CHECK(test == 10000);
    }

    server.terminate();
    seasocksThread.join();
}

#ifndef _WIN32
// Newer Linux kernels reject SO_REUSEPORT on AF_UNIX sockets (issue #208).
// Accepted connections are configured with the same options as the listener,
// so check that a client can actually connect and get a response too.
TEST_CASE("Server can listen on and accept from a unix socket", "[ServerTests]") {
    auto logger = std::make_shared<IgnoringLogger>();
    Server server(logger);
    auto path = "/tmp/seasocks-test-" + std::to_string(::getpid()) + ".sock";
    ::unlink(path.c_str());
    REQUIRE(server.startListeningUnix(path.c_str()));
    std::thread seasocksThread([&] {
        REQUIRE(server.loop());
    });

    int client = ::socket(AF_UNIX, SOCK_STREAM, 0);
    REQUIRE(client != -1);
    timeval timeout{5, 0};
    ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    REQUIRE(::connect(client, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0);

    const std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    REQUIRE(::send(client, request.data(), request.size(), 0) == static_cast<ssize_t>(request.size()));
    char buf[256] = {};
    auto received = ::recv(client, buf, sizeof(buf) - 1, 0);
    // Any HTTP response will do: it shows the accepted socket was usable.
    REQUIRE(received > 0);
    CHECK(std::string(buf, received).rfind("HTTP/1.1 ", 0) == 0);

    ::close(client);
    server.terminate();
    seasocksThread.join();
    ::unlink(path.c_str());
}
#endif
