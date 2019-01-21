// Copyright (c) 2013-2017, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "seasocks/ResponseCode.h"
#include "seasocks/WebSocket.h"
#include "seasocks/ResponseWriter.h"
#include "seasocks/TransferEncoding.h"
#include "seasocks/ZlibContext.h"

#include <netinet/in.h>

#include <sys/socket.h>

#include <cinttypes>
#include <list>
#include <memory>
#include <string>
#include <vector>


namespace seasocks {

class Logger;
class ServerImpl;
class PageRequest;
class Response;

class Connection : public WebSocket {
public:
    Connection(
        std::shared_ptr<Logger> logger,
        ServerImpl& server,
        int fd,
        const sockaddr_in& address);
    virtual ~Connection();

    bool write(const void* data, size_t size, bool flush);
    void handleDataReadyForRead();
    void handleDataReadyForWrite();

    int getFd() const {
        return _fd;
    }

    // From WebSocket.
    virtual void send(const char* webSocketResponse) override;
    virtual void send(const uint8_t* webSocketResponse, size_t length) override;
    virtual void close() override;

    // From Request.
    virtual std::shared_ptr<Credentials> credentials() const override;
    virtual const sockaddr_in& getRemoteAddress() const override {
        return _address;
    }
    virtual const std::string& getRequestUri() const override;
    virtual Request::Verb verb() const override {
        return Request::Verb::WebSocket;
    }
    virtual size_t contentLength() const override {
        return 0;
    }
    virtual const uint8_t* content() const override {
        return nullptr;
    }
    virtual bool hasHeader(const std::string&) const override;
    virtual std::string getHeader(const std::string&) const override;
    virtual Server& server() const override;

    void setLinger();

    size_t inputBufferSize() const {
        return _inBuf.size();
    }
    size_t outputBufferSize() const {
        return _outBuf.size();
    }

    size_t bytesReceived() const {
        return _bytesReceived;
    }
    size_t bytesSent() const {
        return _bytesSent;
    }

    // For testing:
    std::vector<uint8_t>& getInputBuffer() {
        return _inBuf;
    }
    void handleHixieWebSocket();
    void handleHybiWebSocket();
    void setHandler(std::shared_ptr<WebSocket::Handler> handler) {
        _webSocketHandler = handler;
    }
    void handleNewData();


    Connection(Connection& other) = delete;
    Connection& operator=(Connection& other) = delete;


private:
    void finalise();
    bool closed() const;

    void closeWhenEmpty();
    void closeInternal();

    void handleHeaders();
    void handleWebSocketKey3();
    void handleWebSocketTextMessage(const char* message);
    void handleWebSocketBinaryMessage(const std::vector<uint8_t>& message);
    void handleBufferingPostData();
    bool handlePageRequest();

    bool bufferLine(const char* line);
    bool bufferLine(const std::string& line);
    bool flush();

    bool handleHybiHandshake(int webSocketVersion, const std::string& webSocketKey);

    // Send an error document. Returns 'true' for convenience in handle*() routines.
    bool sendError(ResponseCode errorCode, const std::string& document);

    // Send individual errors. Again all return true for convenience.
    bool sendUnsupportedError(const std::string& reason);
    bool send404();
    bool sendBadRequest(const std::string& reason);
    bool sendISE(const std::string& error);

    void sendHybi(uint8_t opcode, const uint8_t* webSocketResponse,
                  size_t messageLength);
    void sendHybiData(const uint8_t* webSocketResponse, size_t messageLength);


    bool sendResponse(std::shared_ptr<Response> response);

    bool processHeaders(uint8_t* first, uint8_t* last);
    bool sendData(const std::string& type, const char* start, size_t size);
    bool sendHeader(const std::string& type, size_t size);

    // Delegated from ResponseWriter.
    struct Writer;
    void begin(ResponseCode responseCode, TransferEncoding encoding);
    void header(const std::string& header, const std::string& value);
    void payload(const void* data, size_t size, bool flush);
    void finish(bool keepConnectionOpen);
    void error(ResponseCode responseCode, const std::string& payload);

    struct Range {
        long start;
        long end;
        size_t length() const {
            return end - start + 1;
        }
    };

    bool parseRange(const std::string& rangeStr, Range& range) const;
    bool parseRanges(const std::string& range, std::list<Range>& ranges) const;
    bool sendStaticData();

    ssize_t safeSend(const void* data, size_t size);

    void bufferResponseAndCommonHeaders(ResponseCode code);

    std::list<Range> processRangesForStaticData(const std::list<Range>& ranges, long fileSize);

    std::shared_ptr<Logger> _logger;
    ServerImpl& _server;
    int _fd;
    bool _shutdown;
    bool _hadSendError;
    bool _closeOnEmpty;
    bool _registeredForWriteEvents;
    sockaddr_in _address;
    size_t _bytesSent;
    size_t _bytesReceived;
    std::vector<uint8_t> _inBuf;
    std::vector<uint8_t> _outBuf;
    std::shared_ptr<WebSocket::Handler> _webSocketHandler;
    bool _shutdownByUser;
    std::unique_ptr<PageRequest> _request;
    std::shared_ptr<Response> _response;
    TransferEncoding _transferEncoding;
    unsigned _chunk;
    std::shared_ptr<Writer> _writer;

    void parsePerMessageDeflateHeader(const std::string& header);
    bool _perMessageDeflate = false;
    ZlibContext zlibContext;

    void pickProtocol();

    enum class State {
        INVALID,
        READING_HEADERS,
        READING_WEBSOCKET_KEY3,
        HANDLING_HIXIE_WEBSOCKET,
        HANDLING_HYBI_WEBSOCKET,
        BUFFERING_POST_DATA,
        AWAITING_RESPONSE_BEGIN,
        SENDING_RESPONSE_HEADERS,
        SENDING_RESPONSE_BODY
    };
    State _state;

    void writeChunkHeader(size_t size);
};

} // namespace seasocks
