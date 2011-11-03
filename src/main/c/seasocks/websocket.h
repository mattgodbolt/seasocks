#ifndef _SEASOCKS_WEBSOCKET_H_
#define _SEASOCKS_WEBSOCKET_H_

#include "seasocks/Request.h"

namespace SeaSocks {

class WebSocket : public Request {
public:
	/**
	 * Send the given data. Must be called on the SeaSocks thread.
	 * See Server::schedule for how to schedule work on the SeaSocks
	 * thread externally.
	 */
	virtual void send(const char* data) = 0;
	/**
	 * Close the socket. It's invalid to access the socket after
	 * calling close(). The Handler::onDisconnect() call may occur
	 * at a later time.
	 */
	virtual void close() = 0;

	/**
	 * Interface to dealing with WebSocket connections.
	 */
	class Handler {
	public:
		virtual ~Handler() { }

		/**
		 * Called on the SeaSocks thread during initial connection.
		 */
		virtual void onConnect(WebSocket* connection) = 0;
		/**
		 * Called on the SeaSocks thread with upon receipt of a full WebSocket message.
		 */
		virtual void onData(WebSocket* connection, const char* data) = 0;
		/**
		 * Called on the SeaSocks thread when the socket has been
		 */
		virtual void onDisconnect(WebSocket* connection) = 0;
	};

protected:
	// To delete a WebSocket, just close it. It is owned by the Server, and
	// the server will delete it when it's finished.
	virtual ~WebSocket() {}
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_WEBSOCKET_H_
