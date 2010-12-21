#ifndef _SEASOCKS_WEBSOCKET_H_
#define _SEASOCKS_WEBSOCKET_H_

namespace SeaSocks {

class WebSocket {
public:
	virtual ~WebSocket() {}
	virtual bool respond(const char* webSocketResponse) = 0;

	class Handler {
	public:
		virtual ~Handler() { }

		virtual void onConnect(WebSocket* connection) = 0;
		virtual void onData(WebSocket* connection, const char* data) = 0;
		virtual void onDisconnect(WebSocket* connection) = 0;
		virtual void onUpdate(WebSocket* connection, void* token) = 0;
	};

};

}  // namespace SeaSocks

#endif  // _SEASOCKS_WEBSOCKET_H_
