#include "logger.h"
#include "printflogger.h"
#include "server.h"
#include "websocket.h"

#include <boost/shared_ptr.hpp>
#include <set>
#include <thread>

/*
 * TODOs:
 * * sort out buffers and buffering
 * * work out threading model for asynch message passing.
 */

using namespace SeaSocks;

void update() {
	for (;;) {
		sleep(1);
		static int i = 0;
		printf("THREAD TICK %d\n", i++);
		// TODO: work out good way of waking up the server to deliver events...
	}
}

class MyHandler : public WebSocket::Handler {
public:
	virtual void onConnect(WebSocket* connection) {
		printf("Got connection\n");
		_connections.insert(connection);
	}

	virtual void onData(WebSocket* connection, const char* data) {
		printf("Got data: '%s'\n", data);
	}

	virtual void onDisconnect(WebSocket* connection) {
		printf("Got disconnection\n");
		_connections.erase(connection);
	}

	virtual void onUpdate(WebSocket* connection, void* token) {
		printf("Got update\n");
	}

private:
	std::set<WebSocket*> _connections;
};

int main(int argc, const char* argv[]) {
	boost::shared_ptr<Logger> logger(new PrintfLogger());
	std::thread otherThread(update);

	Server server(logger);
	server.addWebSocketHandler("/ws", boost::shared_ptr<WebSocket::Handler>(new MyHandler));
	server.serve("src/web", 9090);
	return 0;
}
