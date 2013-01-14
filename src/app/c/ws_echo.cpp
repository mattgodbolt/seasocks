#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

/* Simple server that echo any text or binary WebSocket messages back. */

using namespace seasocks;

class EchoHandler: public WebSocket::Handler {
public:
	virtual void onConnect(WebSocket* connection) {
	}

	virtual void onData(WebSocket* connection, const uint8_t* data, size_t length) {
    connection->send(data, length); // binary
	}

	virtual void onData(WebSocket* connection, const char* data) {
    connection->send(data); // text
	}

	virtual void onDisconnect(WebSocket* connection) {
	}
};

int main(int argc, const char* argv[]) {
	std::shared_ptr<Logger> logger(new PrintfLogger(Logger::DEBUG));

	Server server(logger);
	std::shared_ptr<EchoHandler> handler(new EchoHandler());
	server.addWebSocketHandler("/", handler);
	server.serve("/dev/null", 8000);
	return 0;
}
