#ifndef _SEASOCKS_WEBSOCKET_H_
#define _SEASOCKS_WEBSOCKET_H_

#include <netinet/in.h>
#include <boost/shared_ptr.hpp>
#include "seasocks/credentials.h"

namespace SeaSocks {

class WebSocket {
public:
	virtual ~WebSocket() {}
	virtual bool send(const char* data) = 0;
	virtual void close() = 0;
	virtual boost::shared_ptr<Credentials> credentials() = 0;
	virtual const sockaddr_in& getRemoteAddress() const = 0;

	// Deprecated... use send() instead. 	
	virtual bool respond(const char* data) {
	       return send(data);
      	};

	class Handler {
	public:
		virtual ~Handler() { }

		virtual void onConnect(WebSocket* connection) = 0;
		virtual void onData(WebSocket* connection, const char* data) = 0;
		virtual void onDisconnect(WebSocket* connection) = 0;
	};

};

}  // namespace SeaSocks

#endif  // _SEASOCKS_WEBSOCKET_H_
