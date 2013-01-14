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

using namespace seasocks;

class MyHandler: public WebSocket::Handler {
public:
	MyHandler(Server* server) : _server(server), _currentValue(0) {
		setValue(1);
	}

	virtual void onConnect(WebSocket* connection) {
		_connections.insert(connection);
		connection->send(_currentSetValue.c_str());
    std::cout << "Connected: " << connection->getRequestUri() << " : " << formatAddress(connection->getRemoteAddress()) << std::endl;
		std::cout << "Credentials: " << *(connection->credentials()) << std::endl;
	}

	virtual void onData(WebSocket* connection, const char* data) {
		if (0 == std::strcmp("die", data)) {
		    _server->terminate();
		    return;
		}
		if (0 == std::strcmp("close", data)) {
			std::cout << "Closing.." << std::endl;
		    connection->close();
			std::cout << "Closed." << std::endl;
		    return;
		}

		int value = atoi(data) + 1;
		if (value > _currentValue) {
			setValue(value);
			for (auto iter = _connections.cbegin(); iter != _connections.cend(); ++iter) {
				(*iter)->send(_currentSetValue.c_str());
			}
		}

	}

	virtual void onDisconnect(WebSocket* connection) {
		_connections.erase(connection);
    std::cout << "Disconnected: " << connection->getRequestUri() << " : " << formatAddress(connection->getRemoteAddress()) << std::endl;
	}

private:
	std::set<WebSocket*> _connections;
	Server* _server;
	int _currentValue;
	std::string _currentSetValue;

	void setValue(int value) {
		_currentValue = value;
		std::ostringstream ostr;
		ostr << "set(" << _currentValue << ");";
		_currentSetValue = ostr.str();
	}
};

int main(int argc, const char* argv[]) {
	std::shared_ptr<Logger> logger(new PrintfLogger(Logger::DEBUG));

	Server server(logger);
	server.enableSingleSignOn(SsoOptions::test());

	std::shared_ptr<MyHandler> handler(new MyHandler(&server));
	server.addWebSocketHandler("/ws", handler);
	server.serve("src/ws_test_web", 9090);
	return 0;
}
