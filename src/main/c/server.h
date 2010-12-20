#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

#include <string>

namespace SeaSocks {

class Server {
public:
	Server();
	~Server();

	// Serves static content from the given port on the current thread, forever.
	void serve(const char* staticPath, int port);

private:
	void handleAccept();

	int _listenSock;
	int _epollFd;

	const char* _staticPath;
};


}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
