#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

#include <boost/shared_ptr.hpp>
#include <string>

namespace SeaSocks {

class Logger;

class Server {
public:
	Server(boost::shared_ptr<Logger> logger);
	~Server();

	// Serves static content from the given port on the current thread, forever.
	void serve(const char* staticPath, int port);

private:
	void handleAccept();

	boost::shared_ptr<Logger> _logger;
	int _listenSock;
	int _epollFd;

	const char* _staticPath;
};


}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
