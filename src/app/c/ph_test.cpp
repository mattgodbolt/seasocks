#include "seasocks/printflogger.h"
#include "seasocks/server.h"
#include "seasocks/stringutil.h"
#include "seasocks/PageHandler.h"
#include <string>
#include <cstring>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <set>
#include <iostream>
#include <sstream>

using namespace SeaSocks;

class MyPageHandler: public PageHandler {
public:
	virtual boost::shared_ptr<Response> handleGet(const Request& request) {
		std::ostringstream ostr;
		ostr << "<html><head><title>SeaSocks example</title></head>"
				"<body>You asked for " << request.getRequestUri()
				<< " and your ip is " << request.getRemoteAddress().sin_addr.s_addr
				<< " and a random number is " << rand()
				<< "</body></html";
		return Response::htmlResponse(ostr.str());
	}
};

int main(int argc, const char* argv[]) {
	boost::shared_ptr<Logger> logger(new PrintfLogger(Logger::Level::INFO));

	Server server(logger);
	//server.enableSingleSignOn(SsoOptions::test());
	
	boost::shared_ptr<PageHandler> handler(new MyPageHandler());
	server.setPageHandler(handler);
	server.serve("src/ws_test_web", 9090);
	return 0;
}
