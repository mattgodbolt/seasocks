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
	virtual boost::shared_ptr<Response> handle(const Request& request) {
		if (request.verb() == Request::Post) {
			std::string content(request.content(), request.content() + request.contentLength());
			return Response::textResponse("Thanks for the post. You said: " + content);
		}
		if (request.verb() != Request::Get) return Response::unhandled();
		std::ostringstream ostr;
		ostr << "<html><head><title>SeaSocks example</title></head>"
				"<body>You asked for " << request.getRequestUri()
				<< " and your ip is " << request.getRemoteAddress().sin_addr.s_addr
				<< " and a random number is " << rand()
				<< "<form action=/post method=post><input type=text name=value1></input><br>"
				<< "<input type=submit value=Submit></input></form>"
				<< "</body></html>";
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
