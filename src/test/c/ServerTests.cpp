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

#include <catch2/catch.hpp>

#include <thread>
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
        CHECK(latch == 1);
        CHECK(test == 10000);
    }

    server.terminate();
    seasocksThread.join();
}
