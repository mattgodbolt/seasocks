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
#include "seasocks/util/CrackedUriPageHandler.h"
#include "seasocks/util/RootPageHandler.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
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
    bool loopResult = false; // Only read after the thread is joined.
    std::thread seasocksThread([&] {
        loopResult = server.loop();
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
    CHECK(loopResult);
}

#ifndef _WIN32
namespace {

class UnixSocketServer {
    std::shared_ptr<IgnoringLogger> _logger = std::make_shared<IgnoringLogger>();
    std::string _socketPath = "/tmp/seasocks-test-" + std::to_string(::getpid()) + ".sock";
    std::thread _thread;
    bool _loopResult = false; // Only read after the thread is joined.

public:
    Server server{_logger};

    explicit UnixSocketServer(const std::string& staticPath = "", std::shared_ptr<PageHandler> handler = nullptr,
                              const std::function<void(Server&)>& configure = nullptr) {
        ::unlink(_socketPath.c_str());
        // Not synchronised, so must be set before the server thread starts.
        if (!staticPath.empty()) {
            server.setStaticPath(staticPath.c_str());
        }
        if (handler) {
            server.addPageHandler(std::move(handler));
        }
        if (configure) {
            configure(server);
        }
        REQUIRE(server.startListeningUnix(_socketPath.c_str()));
        _thread = std::thread([this] { _loopResult = server.loop(); });
    }
    ~UnixSocketServer() {
        server.terminate();
        _thread.join();
        CHECK(_loopResult);
        ::unlink(_socketPath.c_str());
    }

    int connectClient() const {
        int client = ::socket(AF_UNIX, SOCK_STREAM, 0);
        REQUIRE(client != -1);
        timeval timeout{5, 0};
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, _socketPath.c_str(), sizeof(addr.sun_path) - 1);
        REQUIRE(::connect(client, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0);
        return client;
    }

    std::string request(const std::string& rawRequest) const {
        int client = connectClient();
        REQUIRE(::send(client, rawRequest.data(), rawRequest.size(), 0) == static_cast<ssize_t>(rawRequest.size()));
        // Half-close so the server closes after replying and we can read to EOF.
        ::shutdown(client, SHUT_WR);
        std::string response;
        char buf[4096];
        for (;;) {
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

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

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

// Issue #152: browsers send an OPTIONS preflight before cross-origin PUT/POST etc.
TEST_CASE("CORS preflight OPTIONS requests are answered", "[ServerTests]") {
    UnixSocketServer s;
    auto response = s.request("OPTIONS /api/thing HTTP/1.1\r\nHost: localhost\r\nOrigin: http://example.com\r\n"
                              "Access-Control-Request-Method: PUT\r\nAccess-Control-Request-Headers: content-type\r\n\r\n");
    CHECK(statusLine(response) == "HTTP/1.1 200 OK");
    CHECK(contains(response, "\r\nAccess-Control-Allow-Origin: *\r\n"));
    CHECK(contains(response, "\r\nAccess-Control-Allow-Methods: GET, HEAD, POST, PUT, DELETE, OPTIONS\r\n"));
    CHECK(contains(response, "\r\nAccess-Control-Allow-Headers: content-type\r\n"));
    CHECK(contains(response, "\r\nAccess-Control-Max-Age: "));
    CHECK(contains(response, "\r\nAllow: GET, HEAD, POST, PUT, DELETE, OPTIONS\r\n"));
    CHECK(contains(response, "\r\nContent-Length: 0\r\n"));
    CHECK(response.substr(response.size() - 4) == "\r\n\r\n");

    SECTION("without a requested header list no Allow-Headers is sent") {
        response = s.request("OPTIONS * HTTP/1.1\r\nHost: localhost\r\n\r\n");
        CHECK(statusLine(response) == "HTTP/1.1 200 OK");
        CHECK(!contains(response, "Access-Control-Allow-Headers"));
    }
}

TEST_CASE("Page handlers can answer OPTIONS requests themselves", "[ServerTests]") {
    struct OptionsHandler : CrackedUriPageHandler {
        std::shared_ptr<Response> handle(const CrackedUri& uri, const Request& request) override {
            if (request.verb() == Request::Verb::Options && uri.path().size() == 1 && uri.path()[0] == "custom") {
                return Response::textResponse("custom-options");
            }
            return Response::unhandled();
        }
    };
    auto root = std::make_shared<RootPageHandler>();
    root->add(std::make_shared<OptionsHandler>());
    UnixSocketServer s("", root);

    auto response = s.request("OPTIONS /custom HTTP/1.1\r\nHost: localhost\r\n\r\n");
    CHECK(contains(response, "custom-options"));
    CHECK(!contains(response, "Access-Control-Allow-Methods"));

    response = s.request("OPTIONS /other HTTP/1.1\r\nHost: localhost\r\n\r\n");
    CHECK(statusLine(response) == "HTTP/1.1 200 OK");
    CHECK(contains(response, "\r\nAccess-Control-Allow-Methods: "));
}

// Issue #203: an endpoint registered without a leading '/' never matched.
TEST_CASE("WebSocket endpoints without a leading slash are registered with one", "[ServerTests]") {
    struct NoOpHandler : WebSocket::Handler {
        void onConnect(WebSocket*) override {
        }
        void onData(WebSocket*, const char*) override {
        }
        void onDisconnect(WebSocket*) override {
        }
    };
    UnixSocketServer s("", nullptr, [](Server& server) {
        server.addWebSocketHandler("pin", std::make_shared<NoOpHandler>());
    });
    auto response = s.request("GET /pin HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                              "Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                              "Sec-WebSocket-Version: 13\r\n\r\n");
    CHECK(statusLine(response).rfind("HTTP/1.1 101 ", 0) == 0);
}

namespace {

// <root>/www is the static dir; <root>/secret.txt and <root>/wwwsecret.txt
// (a sibling sharing its name prefix) must never be served.
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
    SECTION("does not decode percent-encoding when serving files") {
        UnixSocketServer s(tree.www);
        for (const char* uri : {"/%2e%2e/secret.txt", "/a%2f..%2f..%2fsecret.txt"}) {
            INFO(uri);
            auto response = s.get(uri);
            CHECK(statusLine(response) == "HTTP/1.1 404 Not Found");
            CHECK(response.find("TOP-SECRET") == std::string::npos);
        }
    }
    SECTION("a .. after the query marker is not part of the path") {
        UnixSocketServer s(tree.www);
        auto response = s.get("/hello.txt?/../../secret.txt");
        CHECK(statusLine(response) == "HTTP/1.1 200 OK");
        CHECK(response.find("TOP-SECRET") == std::string::npos);
    }
    SECTION("rejects URIs that are not rooted") {
        // "<www>" + "secret.txt" would be the sibling "<root>/wwwsecret.txt".
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
        struct EchoHandler : CrackedUriPageHandler {
            std::shared_ptr<Response> handle(const CrackedUri&, const Request&) override {
                return Response::textResponse("handler-reached");
            }
        };
        auto root = std::make_shared<RootPageHandler>();
        root->add(std::make_shared<EchoHandler>());
        UnixSocketServer s(tree.www, root);
        CHECK(s.get("/anything").find("handler-reached") != std::string::npos);

        auto response = s.get("/../anything");
        CHECK(statusLine(response) == "HTTP/1.1 400 Bad Request");
        CHECK(response.find("handler-reached") == std::string::npos);

        // Percent-encoded: only CrackedUri sees the decoded ".." (and it throws).
        response = s.get("/%2e%2e/anything");
        CHECK(statusLine(response) == "HTTP/1.1 500 Internal Server Error");
        CHECK(response.find("handler-reached") == std::string::npos);
    }
    SECTION("a directory without a trailing slash is a 404, not a truncated 200") {
        REQUIRE(::mkdir((tree.www + "/dir").c_str(), 0700) == 0);
        struct DirRemover {
            std::string path;
            ~DirRemover() {
                ::rmdir(path.c_str());
            }
        } remover{tree.www + "/dir"};
        UnixSocketServer s(tree.www);
        auto response = s.get("/dir");
        CHECK(statusLine(response) == "HTTP/1.1 404 Not Found");
        CHECK(response.find("200 OK") == std::string::npos);
        // A query string is stripped before the lookup, so this is the same directory.
        CHECK(statusLine(s.get("/dir?x=1")) == "HTTP/1.1 404 Not Found");
        // A trailing slash with no index.html is still a plain 404.
        CHECK(statusLine(s.get("/dir/")) == "HTTP/1.1 404 Not Found");
    }
    SECTION("a missing file is a 404 and writes nothing to stdout") {
        UnixSocketServer s(tree.www);
        // Restores fd 1 and releases the capture file even if the request throws.
        struct StdoutCapture {
            int saved;
            std::FILE* file;
            StdoutCapture()
                    : saved(::dup(STDOUT_FILENO)), file(std::tmpfile()) {
                REQUIRE(saved != -1);
                REQUIRE(file != nullptr);
                std::fflush(stdout);
                REQUIRE(::dup2(::fileno(file), STDOUT_FILENO) != -1);
            }
            ~StdoutCapture() {
                std::cout.flush();
                std::fflush(stdout);
                ::dup2(saved, STDOUT_FILENO);
                ::close(saved);
                std::fclose(file);
            }
            long written() const {
                std::cout.flush();
                std::fflush(stdout);
                return ::ftell(file);
            }
        };
        std::string response;
        long written = -1;
        {
            StdoutCapture capture;
            response = s.get("/missing.txt");
            written = capture.written();
        }
        CHECK(statusLine(response) == "HTTP/1.1 404 Not Found");
        CHECK(written == 0);
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

TEST_CASE("Error pages escape the request path and are served as HTML", "[ServerTests]") {
    UnixSocketServer s;
    auto response = s.get("/<script>alert(1)</script>\"'&x");
    CHECK(statusLine(response) == "HTTP/1.1 404 Not Found");
    CHECK(contains(response, "Content-Type: text/html; charset=utf-8\r\n"));
    CHECK_FALSE(contains(response, "<script>"));
    CHECK(contains(response, "Unable to find resource for: /&lt;script&gt;alert(1)&lt;/script&gt;&quot;&#39;&amp;x"));
}

TEST_CASE("Error pages do not re-substitute template placeholders in the body", "[ServerTests]") {
    UnixSocketServer s;
    auto response = s.get("/%%BODY%%%%MESSAGE%%%%ERRORCODE%%");
    CHECK(statusLine(response) == "HTTP/1.1 404 Not Found");
    CHECK(contains(response, "Unable to find resource for: /%%BODY%%%%MESSAGE%%%%ERRORCODE%%"));
}

// Zero-length frames used to index into an empty vector (undefined behaviour).
TEST_CASE("Zero-length WebSocket ping and binary frames are handled", "[ServerTests]") {
    struct EchoHandler : WebSocket::Handler {
        void onConnect(WebSocket*) override {
        }
        void onData(WebSocket* socket, const uint8_t* data, size_t length) override {
            socket->send(data, length);
        }
        void onDisconnect(WebSocket*) override {
        }
    };
    UnixSocketServer s("", nullptr, [](Server& server) {
        server.addWebSocketHandler("/ws", std::make_shared<EchoHandler>());
    });
    const int client = s.connectClient();
    const std::string handshake = "GET /ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                                  "Connection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                                  "Sec-WebSocket-Version: 13\r\n\r\n";
    REQUIRE(::send(client, handshake.data(), handshake.size(), 0) == static_cast<ssize_t>(handshake.size()));
    std::string response;
    while (!contains(response, "\r\n\r\n")) {
        char c;
        REQUIRE(::recv(client, &c, 1, 0) == 1);
        response += c;
    }
    REQUIRE(statusLine(response).rfind("HTTP/1.1 101 ", 0) == 0);

    auto expectFrame = [&](uint8_t header) {
        uint8_t reply[2] = {};
        size_t got = 0;
        while (got < sizeof(reply)) {
            auto n = ::recv(client, reply + got, sizeof(reply) - got, 0);
            REQUIRE(n > 0);
            got += static_cast<size_t>(n);
        }
        CHECK(reply[0] == header);
        CHECK(reply[1] == 0);
    };
    // FIN + opcode, masked with a zero-length payload, then the 4-byte mask key.
    const uint8_t ping[] = {0x89, 0x80, 1, 2, 3, 4};
    const uint8_t binary[] = {0x82, 0x80, 1, 2, 3, 4};
    REQUIRE(::send(client, ping, sizeof(ping), 0) == static_cast<ssize_t>(sizeof(ping)));
    expectFrame(0x8a);
    REQUIRE(::send(client, binary, sizeof(binary), 0) == static_cast<ssize_t>(sizeof(binary)));
    expectFrame(0x82);
    ::close(client);
}

TEST_CASE("Error responses send exactly Content-Length body bytes", "[ServerTests]") {
    UnixSocketServer s;
    auto response = s.get("/missing");
    REQUIRE(statusLine(response) == "HTTP/1.1 404 Not Found");
    const std::string lengthHeader = "\r\nContent-Length: ";
    auto lengthPos = response.find(lengthHeader);
    REQUIRE(lengthPos != std::string::npos);
    const auto declared = std::stoul(response.substr(lengthPos + lengthHeader.size()));
    auto bodyPos = response.find("\r\n\r\n");
    REQUIRE(bodyPos != std::string::npos);
    CHECK(response.size() - (bodyPos + 4) == declared);
}

#ifdef __linux__
namespace {

size_t openFdCount() {
    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        (void) entry;
        ++count;
    }
    return count;
}

} // namespace

TEST_CASE("Failing to start listening does not leak the listen socket", "[ServerTests]") {
    auto logger = std::make_shared<IgnoringLogger>();
    Server server(logger);
    const auto baseline = openFdCount();

    SECTION("unix socket bind failure") {
        for (int i = 0; i < 3; ++i) {
            CHECK_FALSE(server.startListeningUnix("/nonexistent-seasocks-dir/test.sock"));
            CHECK(openFdCount() == baseline);
        }
    }

    SECTION("tcp bind failure") {
        // Occupy a port so that the server's bind() fails.
        int holder = ::socket(AF_INET, SOCK_STREAM, 0);
        REQUIRE(holder != -1);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        REQUIRE(::bind(holder, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0);
        REQUIRE(::listen(holder, 1) == 0);
        socklen_t len = sizeof(addr);
        REQUIRE(::getsockname(holder, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
        const int port = ntohs(addr.sin_port);

        const auto withHolder = openFdCount();
        for (int i = 0; i < 3; ++i) {
            CHECK_FALSE(server.startListening(INADDR_LOOPBACK, port));
            CHECK(openFdCount() == withHolder);
        }
        ::close(holder);
    }

    SECTION("listening again closes the previous listen socket") {
        REQUIRE(server.startListening(INADDR_LOOPBACK, 0));
        const auto listening = openFdCount();
        CHECK(listening == baseline + 1);
        REQUIRE(server.startListening(INADDR_LOOPBACK, 0));
        REQUIRE(server.startListening(INADDR_LOOPBACK, 0));
        CHECK(openFdCount() == listening);
        const std::string path = "/tmp/seasocks-relisten-" + std::to_string(::getpid()) + ".sock";
        for (int i = 0; i < 3; ++i) {
            ::unlink(path.c_str());
            REQUIRE(server.startListeningUnix(path.c_str()));
            CHECK(openFdCount() == listening);
        }
        ::unlink(path.c_str());
    }

    SECTION("a retry on the same server works after a failure") {
        CHECK_FALSE(server.startListeningUnix("/nonexistent-seasocks-dir/test.sock"));
        CHECK(openFdCount() == baseline);
        CHECK(server.startListening(INADDR_LOOPBACK, 0));
        CHECK(openFdCount() == baseline + 1);
    }
}
#endif
#endif
