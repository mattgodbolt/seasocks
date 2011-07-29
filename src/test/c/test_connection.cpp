#include "tinytest.h"

#include <string>
#include "seasocks/connection.h"
#include "seasocks/ignoringlogger.h"
#include <iostream>
#include <sstream>
#include <string.h>

using namespace SeaSocks;

class TestHandler : public WebSocket::Handler {
public:
  int _stage;
  TestHandler() : _stage(0) {}
  ~TestHandler() {
    ASSERT_EQUALS(2, _stage);
  }
  virtual void onConnect(WebSocket*) {}
  virtual void onData(WebSocket*, const char* data) {
    if (_stage == 0) { ASSERT_STRING_EQUALS(data, "a"); }
    else if (_stage == 1) { ASSERT_STRING_EQUALS(data, "b"); }
    else {
      ASSERT_EQUALS(0, 1);
    }
    ++_stage;
  }
  virtual void onDisconnect(WebSocket*) {}
};

void breaks_apart_messages_in_same_buffer() {
  sockaddr_in addr;
  boost::shared_ptr<Logger> logger(new IgnoringLogger);
  Connection connection(logger, NULL, -1, addr, boost::shared_ptr<SsoAuthenticator>());
  connection.setHandler(boost::shared_ptr<WebSocket::Handler>(new TestHandler));
  uint8_t foo[] = { 0x00, 'a', 0xff, 0x00, 'b', 0xff };
  connection.getInputBuffer().assign(&foo[0], &foo[sizeof(foo)]);
  connection.handleWebSocket();
	ASSERT_EQUALS(true, true);
}

int main(int argc, const char* argv[]) {
	RUN(breaks_apart_messages_in_same_buffer);
	return TEST_REPORT();
}
 
