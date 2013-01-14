#include "seasocks/Connection.h"
#include "seasocks/IgnoringLogger.h"

#include <gmock/gmock.h>

#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

using namespace SeaSocks;

class TestHandler: public WebSocket::Handler {
public:
	int _stage;
	TestHandler() :
		_stage(0) {
	}
	~TestHandler() {
	    if (_stage != 2) {
	        ADD_FAILURE() << "Invalid state";
	    }
	}
	virtual void onConnect(WebSocket*) {
	}
	virtual void onData(WebSocket*, const char* data) {
		if (_stage == 0) {
			ASSERT_STREQ(data, "a");
		} else if (_stage == 1) {
		    ASSERT_STREQ(data, "b");
		} else {
			FAIL() << "unexpected state";
		}
		++_stage;
	}
	virtual void onDisconnect(WebSocket*) {
	}
};

TEST(ConnectionTests, shouldBreakHixieMessagesApartInSameBuffer) {
	sockaddr_in addr;
	std::shared_ptr<Logger> logger(new IgnoringLogger);
	Connection connection(logger, NULL, -1, addr, std::shared_ptr<SsoAuthenticator>());
	connection.setHandler(
			std::shared_ptr<WebSocket::Handler>(new TestHandler));
	uint8_t foo[] = { 0x00, 'a', 0xff, 0x00, 'b', 0xff };
	connection.getInputBuffer().assign(&foo[0], &foo[sizeof(foo)]);
	connection.handleHixieWebSocket();
	SUCCEED();
}
