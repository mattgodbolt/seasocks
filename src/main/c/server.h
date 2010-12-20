#ifndef _SEASOCKS_SERVER_H_
#define _SEASOCKS_SERVER_H_

namespace SeaSocks {

class Server {
public:
	Server();
	~Server();

	// Serves content from the given port on the current thread, forever.
	void serve(int port);

private:
	void handleAccept();

	int _listenSock;
	int _epollFd;
};


}  // namespace SeaSocks

#endif  // _SEASOCKS_SERVER_H_
