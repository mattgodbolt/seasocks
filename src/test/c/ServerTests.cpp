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
#include "seasocks/PageHandler.h"
#include "seasocks/Response.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/stat.h>
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
namespace {

// Starts a server listening on a unix socket for the lifetime of the object
// and lets tests send it a raw HTTP request.
class UnixSocketServer {
    std::shared_ptr<IgnoringLogger> _logger = std::make_shared<IgnoringLogger>();
    std::string _socketPath = "/tmp/seasocks-test-" + std::to_string(::getpid()) + ".sock";
    std::thread _thread;

public:
    Server server{_logger};

    explicit UnixSocketServer(const std::string& staticPath = "", std::shared_ptr<PageHandler> handler = nullptr) {
        ::unlink(_socketPath.c_str());
        // Must be set before the server thread starts: it isn't synchronised.
        if (!staticPath.empty()) {
            server.setStaticPath(staticPath.c_str());
        }
        if (handler) {
            server.addPageHandler(std::move(handler));
        }
        REQUIRE(server.startListeningUnix(_socketPath.c_str()));
        _thread = std::thread([this] { REQUIRE(server.loop()); });
    }
    ~UnixSocketServer() {
        server.terminate();
        _thread.join();
        ::unlink(_socketPath.c_str());
    }

    // Returns everything the server sent back before closing or timing out.
    std::string request(const std::string& rawRequest) const {
        int client = ::socket(AF_UNIX, SOCK_STREAM, 0);
        REQUIRE(client != -1);
        timeval timeout{5, 0};
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);
        REQUIRE(::connect(client, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0);
        REQUIRE(::send(client, rawRequest.data(), rawRequest.size(), 0) == static_cast<ssize_t>(rawRequest.size()));
        std::string response;
        char buf[4096];
        // Responses to these requests are small; stop once we have headers
        // and, for 200s, the short body.
        while (response.find("\r\n\r\n") == std::string::npos
               || (response.rfind("HTTP/1.1 200", 0) == 0 && response.find("static-ok") == std::string::npos)) {
            auto received = ::recv(client, buf, sizeof(buf), 0);
            if (received <= 0)
                break;
            response.append(buf, static_cast<size_t>(received));
        }
        ::close(client);
        return response;
    }

    std::string get(const std::string& uri) const {
        return request("GET " + uri + " HTTP/1.1\r\nHost: localhost\r\n\r\n");
    }
};

std::string statusLine(const std::string& response) {
    return response.substr(0, response.find("\r\n"));
}

} // namespace

// Newer Linux kernels reject SO_REUSEPORT on AF_UNIX sockets (issue #208).
// Accepted connections are configured with the same options as the listener,
// so check that a client can actually connect and get a response too.
TEST_CASE("Server can listen on and accept from a unix socket", "[ServerTests]") {
    UnixSocketServer s;
    // Any HTTP response will do: it shows the accepted socket was usable.
    CHECK(s.get("/").rfind("HTTP/1.1 ", 0) == 0);
}

namespace {

// A throwaway directory tree, removed even if an assertion throws:
//   <root>/www/...     the static directory
//   <root>/secret.txt  outside it
//   <root>/wwwsecret.txt  a sibling whose name starts with the static dir's name
struct TempTree {
    std::string root;
    std::string www;
    std::vector<std::string> files;

    TempTree() {
        char rootTemplate[] = "/tmp/seasocks-static-XXXXXX";
        REQUIRE(::mkdtemp(rootTemplate) != nullptr);
        root = rootTemplate;
        www = root + "/www";
        REQUIRE(::mkdir(www.c_str(), 0700) == 0);
    }
    ~TempTree() {
        for (const auto& f : files) {
            std::remove(f.c_str());
        }
        ::rmdir(www.c_str());
        ::rmdir(root.c_str());
    }
    void write(const std::string& path, const std::string& contents) {
        files.push_back(path);
        std::ofstream out(path);
        out << contents;
        REQUIRE(out.good());
    }
};

} // namespace

TEST_CASE("Static file serving stays inside the static path", "[ServerTests]") {
    TempTree tree;
    tree.write(tree.www + "/hello.txt", "static-ok");
    tree.write(tree.root + "/secret.txt", "TOP-SECRET");
    tree.write(tree.root + "/wwwsecret.txt", "TOP-SECRET");

    SECTION("with no static path nothing is served from the filesystem") {
        UnixSocketServer s;
        CHECK(statusLine(s.get("/etc/passwd")) == "HTTP/1.1 404 Not Found");
        CHECK(statusLine(s.get(tree.root + "/secret.txt")) == "HTTP/1.1 404 Not Found");
    }
    SECTION("serves files under the static path") {
        UnixSocketServer s(tree.www);
        auto response = s.get("/hello.txt");
        CHECK(statusLine(response) == "HTTP/1.1 200 OK");
        CHECK(response.find("static-ok") != std::string::npos);
    }
    SECTION("rejects parent directory segments") {
        UnixSocketServer s(tree.www);
        for (const char* uri : {"/../secret.txt", "/a/../../secret.txt", "/..", "/../", "/..\\secret.txt",
                                "/hello.txt/../../secret.txt", "/..?x=1"}) {
            INFO(uri);
            auto response = s.get(uri);
            CHECK(statusLine(response) == "HTTP/1.1 400 Bad Request");
            CHECK(response.find("TOP-SECRET") == std::string::npos);
        }
    }
    SECTION("a .. after the query marker is not part of the path") {
        // The path is /hello.txt; what follows '?' is trimmed before opening the file.
        UnixSocketServer s(tree.www);
        auto response = s.get("/hello.txt?/../../secret.txt");
        CHECK(statusLine(response) == "HTTP/1.1 200 OK");
        CHECK(response.find("TOP-SECRET") == std::string::npos);
    }
    SECTION("rejects URIs that are not rooted") {
        // Without a leading '/', "<www>" + "secret.txt" would be the sibling "<root>/wwwsecret.txt".
        UnixSocketServer s(tree.www);
        auto response = s.get("secret.txt");
        CHECK(statusLine(response) == "HTTP/1.1 400 Bad Request");
        CHECK(response.find("TOP-SECRET") == std::string::npos);
    }
    SECTION("allows dots that are not a parent segment") {
        tree.write(tree.www + "/a..b.txt", "static-ok");
        UnixSocketServer s(tree.www);
        CHECK(statusLine(s.get("/a..b.txt")) == "HTTP/1.1 200 OK");
        CHECK(statusLine(s.get("/hello.txt?next=../x")) == "HTTP/1.1 200 OK");
    }
    SECTION("rejects parent directory segments before custom handlers see them") {
        struct EchoHandler : PageHandler {
            std::shared_ptr<Response> handle(const Request&) override {
                return Response::textResponse("handler-reached");
            }
        };
        UnixSocketServer s(tree.www, std::make_shared<EchoHandler>());
        auto ok = s.get("/anything");
        CHECK(ok.find("handler-reached") != std::string::npos);
        auto response = s.get("/../anything");
        CHECK(statusLine(response) == "HTTP/1.1 400 Bad Request");
        CHECK(response.find("handler-reached") == std::string::npos);
    }
    SECTION("applies to HEAD and POST too") {
        UnixSocketServer s(tree.www);
        for (const char* verb : {"HEAD", "POST"}) {
            INFO(verb);
            auto response = s.request(std::string(verb) + " /../secret.txt HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n");
            CHECK(statusLine(response) == "HTTP/1.1 400 Bad Request");
            CHECK(response.find("TOP-SECRET") == std::string::npos);
        }
    }
}
#endif
