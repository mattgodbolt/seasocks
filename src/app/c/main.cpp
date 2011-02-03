#include "seasocks/logger.h"
#include "seasocks/printflogger.h"
#include "seasocks/server.h"
#include "seasocks/websocket.h"

#include <boost/shared_ptr.hpp>
#include <set>
#include <thread>

/*
 * TODOs:
 * * sort out buffers and buffering
 * * work out threading model for asynch message passing.
 */

using namespace SeaSocks;

class MyHandler;
boost::shared_ptr<MyHandler> handler;

class MyHandler : public WebSocket::Handler {
public:
	MyHandler(Server* server) : _server(server) {
	}

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

	class SendToAllRunnable : public Server::Runnable {
	public:
		SendToAllRunnable(boost::shared_ptr<MyHandler> handler, const char* message)
			: _handler(handler), _message(message) {
		}

		virtual void run() {
			for (auto iter = _handler->_connections.cbegin(); iter != _handler->_connections.cend(); ++iter) {
				(*iter)->respond(_message.c_str());
			}
		}
	private:
		boost::shared_ptr<MyHandler> _handler;
		std::string _message;
	};

	void sendToAll(const char* message) {
		boost::shared_ptr<Server::Runnable> runnable(new SendToAllRunnable(handler, message));
		_server->schedule(runnable);
	}

private:
	std::set<WebSocket*> _connections;
	Server* _server;
};

void update() {
	for (;;) {
		sleep(1);
		static int i = 0;
		printf("THREAD TICK %d\n", i++);
		char buf[128];
		sprintf(buf, "console.log('%d');", i);
		handler->sendToAll(buf);
	}
}

int main(int argc, const char* argv[]) {
	boost::shared_ptr<Logger> logger(new PrintfLogger());

	std::thread otherThread(update);

	Server server(logger);
	handler.reset(new MyHandler(&server));
	server.addWebSocketHandler("/ws", handler);
	server.serve("src/web", 9090);
	return 0;
}
