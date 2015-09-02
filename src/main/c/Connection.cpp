// Copyright (c) 2013, Matt Godbolt
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

#include "internal/Config.h"
#include "internal/Embedded.h"
#include "internal/HeaderMap.h"
#include "internal/HybiAccept.h"
#include "internal/HybiPacketDecoder.h"
#include "internal/LogStream.h"
#include "internal/PageRequest.h"
#include "internal/Version.h"

#include "md5/md5.h"

#include "seasocks/Connection.h"
#include "seasocks/Credentials.h"
#include "seasocks/Logger.h"
#include "seasocks/PageHandler.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/ToString.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <unordered_map>

namespace {

uint32_t parseWebSocketKey(const std::string& key) {
    uint32_t keyNumber = 0;
    uint32_t numSpaces = 0;
    for (auto c : key) {
        if (c >= '0' && c <= '9') {
            keyNumber = keyNumber * 10 + c - '0';
        } else if (c == ' ') {
            ++numSpaces;
        }
    }
    return numSpaces > 0 ? keyNumber / numSpaces : 0;
}

char* extractLine(uint8_t*& first, uint8_t* last, char** colon = NULL) {
    for (uint8_t* ptr = first; ptr < last - 1; ++ptr) {
        if (ptr[0] == '\r' && ptr[1] == '\n') {
            ptr[0] = 0;
            uint8_t* result = first;
            first = ptr + 2;
            return reinterpret_cast<char*> (result);
        }
        if (colon && ptr[0] == ':' && *colon == NULL) {
            *colon = reinterpret_cast<char*> (ptr);
        }
    }
    return NULL;
}

std::string webtime(time_t time) {
    struct tm tm;
    gmtime_r(&time, &tm);
    char buf[1024];
    // Wed, 20 Apr 2011 17:31:28 GMT
    strftime(buf, sizeof(buf)-1, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return buf;
}

std::string now() {
    return webtime(time(NULL));
}

class RaiiFd {
    int _fd;
public:
    RaiiFd(const char* filename) {
        _fd = ::open(filename, O_RDONLY);
    }
    RaiiFd(const RaiiFd&) = delete;
    RaiiFd& operator=(const RaiiFd&) = delete;
    ~RaiiFd() {
        if (_fd != -1) {
            ::close(_fd);
        }
    }
    bool ok() const {
        return _fd != -1;
    }
    operator int() const {
        return _fd;
    }
};

const std::unordered_map<std::string, std::string> contentTypes = {
    { "txt", "text/plain" },
    { "css", "text/css" },
    { "csv", "text/csv" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "xml", "text/xml" },
    { "js", "text/javascript" }, // Technically it should be application/javascript (RFC 4329), but IE8 struggles with that
    { "xhtml", "application/xhtml+xml" },
    { "json", "application/json" },
    { "pdf", "application/pdf" },
    { "zip", "application/zip" },
    { "tar", "application/x-tar" },
    { "gif", "image/gif" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "tiff", "image/tiff" },
    { "tif", "image/tiff" },
    { "png", "image/png" },
    { "svg", "image/svg+xml" },
    { "ico", "image/x-icon" },
    { "swf", "application/x-shockwave-flash" },
    { "mp3", "audio/mpeg" },
    { "wav", "audio/x-wav" },
    { "ttf", "font/ttf" },
};

std::string getExt(const std::string& path) {
    auto lastDot = path.find_last_of('.');
    if (lastDot != path.npos) {
        return path.substr(lastDot + 1);
    }
    return "";
}

const std::string& getContentType(const std::string& path) {
    auto it = contentTypes.find(getExt(path));
    if (it != contentTypes.end()) {
        return it->second;
    }
    static const std::string defaultType("text/html");
    return defaultType;
}

// Cacheability is only set for resources that *REQUIRE* caching for browser support reasons.
// It's off for everything else to save on browser reload headaches during development, at
// least until we support ETags or If-Modified-Since: type checking, which we may never do.
bool isCacheable(const std::string& path) {
    std::string extension = getExt(path);
    if (extension == "mp3" || extension == "wav") {
        return true;
    }
    return false;
}

const size_t MaxBufferSize = 16 * 1024 * 1024;
const size_t ReadWriteBufferSize = 16 * 1024;
const size_t MaxWebsocketMessageSize = 16384;
const size_t MaxHeadersSize = 64 * 1024;

class PrefixWrapper : public seasocks::Logger {
    std::string _prefix;
    std::shared_ptr<Logger> _logger;
public:
    PrefixWrapper(const std::string& prefix, std::shared_ptr<Logger> logger)
    : _prefix(prefix), _logger(logger) {}

    virtual void log(Level level, const char* message) {
        _logger->log(level, (_prefix + message).c_str());
    }
};

bool hasConnectionType(const std::string &connection, const std::string &type) {
    for (auto conType : seasocks::split(connection, ',')) {
        while (!conType.empty() && isspace(conType[0]))
            conType = conType.substr(1);
        if (seasocks::caseInsensitiveSame(conType, type))
            return true;
    }
    return false;
}

}  // namespace

namespace seasocks {

Connection::Connection(
        std::shared_ptr<Logger> logger,
        ServerImpl& server,
        int fd,
        const sockaddr_in& address)
    : _logger(new PrefixWrapper(formatAddress(address) + " : ", logger)),
      _server(server),
      _fd(fd),
      _shutdown(false),
      _hadSendError(false),
      _closeOnEmpty(false),
      _registeredForWriteEvents(false),
      _address(address),
      _bytesSent(0),
      _bytesReceived(0),
      _shutdownByUser(false),
      _state(READING_HEADERS) {
}

Connection::~Connection() {
    _server.checkThread();
    finalise();
}

void Connection::close() {
    // This is the user-side close requests ONLY! You should Call closeInternal
    _shutdownByUser = true;
    closeInternal();
}

void Connection::closeWhenEmpty() {
    if (_outBuf.empty()) {
        closeInternal();
    } else {
        _closeOnEmpty = true;
    }
}

void Connection::closeInternal() {
    // It only actually only calls shutdown on the socket,
    // leaving the close of the FD and the cleanup until the destructor runs.
    _server.checkThread();
    if (_fd != -1 && !_shutdown && ::shutdown(_fd, SHUT_RDWR) == -1) {
        LS_WARNING(_logger, "Unable to shutdown socket : " << getLastError());
    }
    _shutdown = true;
}


void Connection::finalise() {
    if (_webSocketHandler) {
        _webSocketHandler->onDisconnect(this);
        _webSocketHandler.reset();
    }
    if (_fd != -1) {
        _server.remove(this);
        LS_DEBUG(_logger, "Closing socket");
        ::close(_fd);
    }
    _fd = -1;
}

int Connection::safeSend(const void* data, size_t size) {
    if (_fd == -1 || _hadSendError || _shutdown) {
        // Ignore further writes to the socket, it's already closed or has been shutdown
        return -1;
    }
    int sendResult = ::send(_fd, data, size, MSG_NOSIGNAL);
    if (sendResult == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Treat this as if zero bytes were written.
            return 0;
        }
        LS_WARNING(_logger, "Unable to write to socket : " << getLastError() << " - disabling further writes");
        closeInternal();
    } else {
        _bytesSent += sendResult;
    }
    return sendResult;
}

bool Connection::write(const void* data, size_t size, bool flushIt) {
    if (closed() || _closeOnEmpty) {
        return false;
    }
    if (size) {
        int bytesSent = 0;
        if (_outBuf.empty() && flushIt) {
            // Attempt fast path, send directly.
            bytesSent = safeSend(data, size);
            if (bytesSent == static_cast<int>(size)) {
                // We sent directly.
                return true;
            }
            if (bytesSent == -1) {
                return false;
            }
        }
        size_t bytesToBuffer = size - bytesSent;
        size_t endOfBuffer = _outBuf.size();
        size_t newBufferSize = endOfBuffer + bytesToBuffer;
        if (newBufferSize >= MaxBufferSize) {
            LS_WARNING(_logger, "Closing connection: buffer size too large (" 
                    << newBufferSize << " >= " << MaxBufferSize << ")");
            closeInternal();
            return false;
        }
        _outBuf.resize(newBufferSize);
        memcpy(&_outBuf[endOfBuffer], reinterpret_cast<const uint8_t*>(data) + bytesSent, bytesToBuffer);
    }
    if (flushIt) {
        return flush();
    }
    return true;
}

bool Connection::bufferLine(const char* line) {
    static const char crlf[] = { '\r', '\n' };
    if (!write(line, strlen(line), false)) return false;
    return write(crlf, 2, false);
}

bool Connection::bufferLine(const std::string& line) {
    std::string lineAndCrlf = line + "\r\n";
    return write(lineAndCrlf.c_str(), lineAndCrlf.length(), false);
}

void Connection::handleDataReadyForRead() {
    if (closed()) {
        return;
    }
    size_t curSize = _inBuf.size();
    _inBuf.resize(curSize + ReadWriteBufferSize);
    int result = ::read(_fd, &_inBuf[curSize], ReadWriteBufferSize);
    if (result == -1) {
        LS_WARNING(_logger, "Unable to read from socket : " << getLastError());
        return;
    }
    if (result == 0) {
        LS_DEBUG(_logger, "Remote end closed connection");
        closeInternal();
        return;
    }
    _bytesReceived += result;
    _inBuf.resize(curSize + result);
    handleNewData();
}

void Connection::handleDataReadyForWrite() {
    if (closed()) {
        return;
    }
    flush();
}

bool Connection::flush() {
    if (_outBuf.empty()) {
        return true;
    }
    int numSent = safeSend(&_outBuf[0], _outBuf.size());
    if (numSent == -1) {
        return false;
    }
    _outBuf.erase(_outBuf.begin(), _outBuf.begin() + numSent);
    if (_outBuf.size() > 0 && !_registeredForWriteEvents) {
        if (!_server.subscribeToWriteEvents(this)) {
            return false;
        }
        _registeredForWriteEvents = true;
    } else if (_outBuf.empty() && _registeredForWriteEvents) {
        if (!_server.unsubscribeFromWriteEvents(this)) {
            return false;
        }
        _registeredForWriteEvents = false;
    }
    if (_outBuf.empty() && !closed() && _closeOnEmpty) {
        LS_DEBUG(_logger, "Ready for close, now empty");
        closeInternal();
    }
    return true;
}

bool Connection::closed() const {
    return _fd == -1 || _shutdown;
}

void Connection::handleNewData() {
    switch (_state) {
    case READING_HEADERS:
        handleHeaders();
        break;
    case READING_WEBSOCKET_KEY3:
        handleWebSocketKey3();
        break;
    case HANDLING_HIXIE_WEBSOCKET:
        handleHixieWebSocket();
        break;
    case HANDLING_HYBI_WEBSOCKET:
        handleHybiWebSocket();
        break;
    case BUFFERING_POST_DATA:
        handleBufferingPostData();
        break;
    default:
        assert(false);
        break;
    }
}

void Connection::handleHeaders() {
    if (_inBuf.size() < 4) {
        return;
    }
    for (size_t i = 0; i <= _inBuf.size() - 4; ++i) {
        if (_inBuf[i] == '\r' &&
            _inBuf[i+1] == '\n' &&
            _inBuf[i+2] == '\r' &&
            _inBuf[i+3] == '\n') {
            if (!processHeaders(&_inBuf[0], &_inBuf[i + 2])) {
                closeInternal();
                return;
            }
            _inBuf.erase(_inBuf.begin(), _inBuf.begin() + i + 4);
            handleNewData();
            return;
        }
    }
    if (_inBuf.size() > MaxHeadersSize) {
        sendUnsupportedError("Headers too big");
    }
}

void Connection::handleWebSocketKey3() {
    constexpr auto WebSocketKeyLen = 8u;
    if (_inBuf.size() < WebSocketKeyLen) {
        return;
    }

    struct {
        uint32_t key1;
        uint32_t key2;
        char key3[WebSocketKeyLen];
    } md5Source;

    auto key1 = parseWebSocketKey(_request->getHeader("Sec-WebSocket-Key1"));
    auto key2 = parseWebSocketKey(_request->getHeader("Sec-WebSocket-Key2"));

    LS_DEBUG(_logger, "Got a hixie websocket with key1=0x" << std::hex << key1 << ", key2=0x" << key2);

    md5Source.key1 = htonl(key1);
    md5Source.key2 = htonl(key2);
    memcpy(&md5Source.key3, &_inBuf[0], WebSocketKeyLen);

    uint8_t digest[16];
    md5_state_t md5state;
    md5_init(&md5state);
    md5_append(&md5state, reinterpret_cast<const uint8_t*>(&md5Source), sizeof(md5Source));
    md5_finish(&md5state, digest);

    LS_DEBUG(_logger, "Attempting websocket upgrade");

    bufferResponseAndCommonHeaders(ResponseCode::WebSocketProtocolHandshake);
    bufferLine("Upgrade: websocket");
    bufferLine("Connection: Upgrade");
    bool allowCrossOrigin = _server.isCrossOriginAllowed(_request->getRequestUri());
    if (_request->hasHeader("Origin") && allowCrossOrigin) {
        bufferLine("Sec-WebSocket-Origin: " +  _request->getHeader("Origin"));
    }
    if (_request->hasHeader("Host")) {
        auto host = _request->getHeader("Host");
        if (!allowCrossOrigin) {
            bufferLine("Sec-WebSocket-Origin: http://" + host);
        }
        bufferLine("Sec-WebSocket-Location: ws://" + host + _request->getRequestUri());
    }
    bufferLine("");

    write(&digest, 16, true);

    _state = HANDLING_HIXIE_WEBSOCKET;
    _inBuf.erase(_inBuf.begin(), _inBuf.begin() + 8);
    if (_webSocketHandler) {
        _webSocketHandler->onConnect(this);
    }
}

void Connection::handleBufferingPostData() {
    if (_request->consumeContent(_inBuf)) {
        _state = READING_HEADERS;
        if (!handlePageRequest()) {
            closeInternal();
        }
    }
}

void Connection::send(const char* webSocketResponse) {
    _server.checkThread();
    if (_shutdown) {
        if (_shutdownByUser) {
            LS_ERROR(_logger, "Server wrote to connection after closing it");
        }
        return;
    }
    auto messageLength = strlen(webSocketResponse);
    if (_state == HANDLING_HIXIE_WEBSOCKET) {
        uint8_t zero = 0;
        if (!write(&zero, 1, false)) return;
        if (!write(webSocketResponse, messageLength, false)) return;
        uint8_t effeff = 0xff;
        write(&effeff, 1, true);
        return;
    }
    sendHybi(HybiPacketDecoder::OPCODE_TEXT, reinterpret_cast<const uint8_t*>(webSocketResponse), messageLength);
}

void Connection::send(const uint8_t* data, size_t length) {
    _server.checkThread();
    if (_shutdown) {
        if (_shutdownByUser) {
            LS_ERROR(_logger, "Client wrote to connection after closing it");
        }
        return;
    }
    if (_state == HANDLING_HIXIE_WEBSOCKET) {
        LS_ERROR(_logger, "Hixie does not support binary");
        return;
    }
    sendHybi(HybiPacketDecoder::OPCODE_BINARY, data, length);
}

void Connection::sendHybi(int opcode, const uint8_t* webSocketResponse, size_t messageLength) {
    uint8_t firstByte = 0x80 | opcode;
    if (!write(&firstByte, 1, false)) return;
    if (messageLength < 126) {
        uint8_t nextByte = messageLength; // No MASK bit set.
        if (!write(&nextByte, 1, false)) return;
    } else if (messageLength < 65536) {
        uint8_t nextByte = 126; // No MASK bit set.
        if (!write(&nextByte, 1, false)) return;
        auto lengthBytes = htons(messageLength);
        if (!write(&lengthBytes, 2, false)) return;
    } else {
        uint8_t nextByte = 127; // No MASK bit set.
        if (!write(&nextByte, 1, false)) return;
        uint64_t lengthBytes = __bswap_64(messageLength);
        if (!write(&lengthBytes, 8, false)) return;
    }
    write(webSocketResponse, messageLength, true);
}

std::shared_ptr<Credentials> Connection::credentials() const {
    _server.checkThread();
    return _request ? _request->credentials() : std::shared_ptr<Credentials>();
}

void Connection::handleHixieWebSocket() {
    if (_inBuf.empty()) {
        return;
    }
    size_t messageStart = 0;
    while (messageStart < _inBuf.size()) {
        if (_inBuf[messageStart] != 0) {
            LS_WARNING(_logger, "Error in WebSocket input stream (got " << (int)_inBuf[messageStart] << ")");
            closeInternal();
            return;
        }
        // TODO: UTF-8
        size_t endOfMessage = 0;
        for (size_t i = messageStart + 1; i < _inBuf.size(); ++i) {
            if (_inBuf[i] == 0xff) {
                endOfMessage = i;
                break;
            }
        }
        if (endOfMessage != 0) {
            _inBuf[endOfMessage] = 0;
            handleWebSocketTextMessage(reinterpret_cast<const char*>(&_inBuf[messageStart + 1]));
            messageStart = endOfMessage + 1;
        } else {
            break;
        }
    }
    if (messageStart != 0) {
        _inBuf.erase(_inBuf.begin(), _inBuf.begin() + messageStart);
    }
    if (_inBuf.size() > MaxWebsocketMessageSize) {
        LS_WARNING(_logger, "WebSocket message too long");
        closeInternal();
    }
}

void Connection::handleHybiWebSocket() {
    if (_inBuf.empty()) {
        return;
    }
    HybiPacketDecoder decoder(*_logger, _inBuf);
    bool done = false;
    while (!done) {
        std::vector<uint8_t> decodedMessage;
        switch (decoder.decodeNextMessage(decodedMessage)) {
        default:
            closeInternal();
            LS_WARNING(_logger, "Unknown HybiPacketDecoder state");
            return;
        case HybiPacketDecoder::Error:
            closeInternal();
            return;
        case HybiPacketDecoder::TextMessage:
            decodedMessage.push_back(0);  // avoids a copy
            handleWebSocketTextMessage(reinterpret_cast<const char*>(&decodedMessage[0]));
            break;
        case HybiPacketDecoder::BinaryMessage:
            handleWebSocketBinaryMessage(decodedMessage);
            break;
        case HybiPacketDecoder::Ping:
            sendHybi(HybiPacketDecoder::OPCODE_PONG, &decodedMessage[0], decodedMessage.size());
            break;
        case HybiPacketDecoder::NoMessage:
            done = true;
            break;
        case HybiPacketDecoder::Close:
            LS_DEBUG(_logger, "Received WebSocket close");
            closeInternal();
            return;
        }
    }
    if (decoder.numBytesDecoded() != 0) {
        _inBuf.erase(_inBuf.begin(), _inBuf.begin() + decoder.numBytesDecoded());
    }
    if (_inBuf.size() > MaxWebsocketMessageSize) {
        LS_WARNING(_logger, "WebSocket message too long");
        closeInternal();
    }
}

void Connection::handleWebSocketTextMessage(const char* message) {
    LS_DEBUG(_logger, "Got text web socket message: '" << message << "'");
    if (_webSocketHandler) {
        _webSocketHandler->onData(this, message);
    }
}

void Connection::handleWebSocketBinaryMessage(const std::vector<uint8_t>& message) {
    LS_DEBUG(_logger, "Got binary web socket message (size: " << message.size() << ")");
    if (_webSocketHandler) {
        _webSocketHandler->onData(this, &message[0], message.size());
    }
}

bool Connection::sendError(ResponseCode errorCode, const std::string& body) {
    assert(_state != HANDLING_HIXIE_WEBSOCKET);
    auto errorNumber = static_cast<int>(errorCode);
    auto message = ::name(errorCode);
    bufferResponseAndCommonHeaders(errorCode);
    auto errorContent = findEmbeddedContent("/_error.html");
    std::string document;
    if (errorContent) {
        document.assign(errorContent->data, errorContent->data + errorContent->length);
        replace(document, "%%ERRORCODE%%", toString(errorNumber));
        replace(document, "%%MESSAGE%%", message);
        replace(document, "%%BODY%%", body);
    } else {
        std::stringstream documentStr;
        documentStr << "<html><head><title>" << errorNumber << " - " << message << "</title></head>"
                << "<body><h1>" << errorNumber << " - " << message << "</h1>"
                << "<div>" << body << "</div><hr/><div><i>Powered by seasocks</i></div></body></html>";
        document = documentStr.str();
    }
    bufferLine("Content-Length: " + toString(document.length()));
    bufferLine("Connection: close");
    bufferLine("");
    bufferLine(document);
    if (!flush()) {
        return false;
    }
    closeWhenEmpty();
    return true;
}

bool Connection::sendUnsupportedError(const std::string& reason) {
    return sendError(ResponseCode::NotImplemented, reason);
}

bool Connection::send404() {
    auto path = getRequestUri();
    auto embedded = findEmbeddedContent(path);
    if (embedded) {
        return sendData(getContentType(path), embedded->data, embedded->length);
    } else if (strcmp(path.c_str(), "/_livestats.js") == 0) {
        auto stats = _server.getStatsDocument();
        return sendData("text/javascript", stats.c_str(), stats.length());
    } else {
        return sendError(ResponseCode::NotFound, "Unable to find resource for: " + path);
    }
}

bool Connection::sendBadRequest(const std::string& reason) {
    return sendError(ResponseCode::BadRequest, reason);
}

bool Connection::sendISE(const std::string& error) {
    return sendError(ResponseCode::InternalServerError, error);
}

bool Connection::processHeaders(uint8_t* first, uint8_t* last) {
    // Ideally we'd copy off [first, last] now into a header structure here.
    // Be careful about lifetimes though and multiple requests coming in, should
    // we ever support HTTP pipelining and/or long-lived requests.
    char* requestLine = extractLine(first, last);
    assert(requestLine != NULL);

    LS_ACCESS(_logger, "Request: " << requestLine);

    const char* verbText = shift(requestLine);
    if (!verbText) {
        return sendBadRequest("Malformed request line");
    }
    auto verb = Request::verb(verbText);
    if (verb == Request::Verb::Invalid) {
        return sendBadRequest("Malformed request line");
    }
    const char* requestUri = shift(requestLine);
    if (requestUri == NULL) {
        return sendBadRequest("Malformed request line");
    }

    const char* httpVersion = shift(requestLine);
    if (httpVersion == NULL) {
        return sendBadRequest("Malformed request line");
    }
    if (strcmp(httpVersion, "HTTP/1.1") != 0) {
        return sendUnsupportedError("Unsupported HTTP version");
    }
    if (*requestLine != 0) {
        return sendBadRequest("Trailing crap after http version");
    }

    HeaderMap headers(31);
    while (first < last) {
        char* colonPos = NULL;
        char* headerLine = extractLine(first, last, &colonPos);
        assert(headerLine != NULL);
        if (colonPos == NULL) {
            return sendBadRequest("Malformed header");
        }
        *colonPos = 0;
        const char* key = headerLine;
        const char* value = skipWhitespace(colonPos + 1);
        LS_DEBUG(_logger, "Key: " << key << " || " << value);
#if HAVE_UNORDERED_MAP_EMPLACE
        headers.emplace(key, value);
#else
        headers.insert(std::make_pair(key, value));
#endif
    }

    if (headers.count("Connection") && headers.count("Upgrade")
            && hasConnectionType(headers["Connection"], "Upgrade")
            && caseInsensitiveSame(headers["Upgrade"], "websocket")) {
        LS_INFO(_logger, "Websocket request for " << requestUri << "'");
        if (verb != Request::Verb::Get) {
            return sendBadRequest("Non-GET WebSocket request");
        }
        _webSocketHandler = _server.getWebSocketHandler(requestUri);
        if (!_webSocketHandler) {
            LS_WARNING(_logger, "Couldn't find WebSocket end point for '" << requestUri << "'");
            return send404();
        }
        verb = Request::Verb::WebSocket;
    }

    _request.reset(new PageRequest(_address, requestUri, verb, std::move(headers)));

    const EmbeddedContent *embedded = findEmbeddedContent(requestUri);
    if (verb == Request::Verb::Get && embedded) {
        // MRG: one day, this could be a request handler.
        return sendData(getContentType(requestUri), embedded->data, embedded->length);
    }

    if (_request->contentLength() > MaxBufferSize) {
        return sendBadRequest("Content length too long");
    }
    if (_request->contentLength() == 0) {
        return handlePageRequest();
    }
    _state = BUFFERING_POST_DATA;
    return true;
}

bool Connection::handlePageRequest() {
    std::shared_ptr<Response> response;
    try {
        response = _server.handle(*_request);
    } catch (const std::exception& e) {
        LS_ERROR(_logger, "page error: " << e.what());
        return sendISE(e.what());
    } catch (...) {
        LS_ERROR(_logger, "page error: (unknown)");
        return sendISE("(unknown)");
    }
    auto uri = _request->getRequestUri();
    if (!response && _request->verb() == Request::Verb::WebSocket) {
        _webSocketHandler = _server.getWebSocketHandler(uri.c_str());
        auto webSocketVersion = atoi(_request->getHeader("Sec-WebSocket-Version").c_str());
        if (!_webSocketHandler) {
            LS_WARNING(_logger, "Couldn't find WebSocket end point for '" << uri << "'");
            return send404();
        }
        if (webSocketVersion == 0) {
            // Hixie
            _state = READING_WEBSOCKET_KEY3;
            return true;
        }
        auto hybiKey = _request->getHeader("Sec-WebSocket-Key");
        return handleHybiHandshake(webSocketVersion, hybiKey);
    }
    return sendResponse(response);
}

bool Connection::sendResponse(std::shared_ptr<Response> response) {
    const auto requestUri = _request->getRequestUri();
    if (response == Response::unhandled()) {
        return sendStaticData();
    }
    if (response->responseCode() == ResponseCode::NotFound) {
        // TODO: better here; we use this purely to serve our own embedded content.
        return send404();
    } else if (!isOk(response->responseCode())) {
        return sendError(response->responseCode(), response->payload());
    }

    bufferResponseAndCommonHeaders(response->responseCode());
    bufferLine("Content-Length: " + toString(response->payloadSize()));
    bufferLine("Content-Type: " + response->contentType());
    if (response->keepConnectionAlive()) {
        bufferLine("Connection: keep-alive");
    } else {
        bufferLine("Connection: close");
    }
    bufferLine("Last-Modified: " + now());
    bufferLine("Cache-Control: no-store");
    bufferLine("Pragma: no-cache");
    bufferLine("Expires: " + now());
    auto headers = response->getAdditionalHeaders();
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        bufferLine(it->first + ": " + it->second);
    }
    bufferLine("");

    if (!write(response->payload(), response->payloadSize(), true)) {
        return false;
    }
    if (!response->keepConnectionAlive()) {
        closeWhenEmpty();
    }
    return true;
}

bool Connection::handleHybiHandshake(
        int webSocketVersion,
        const std::string& webSocketKey) {
    if (webSocketVersion != 8 && webSocketVersion != 13) {
        return sendBadRequest("Invalid websocket version");
    }
    LS_DEBUG(_logger, "Got a hybi-8 websocket with key=" << webSocketKey);

    LS_DEBUG(_logger, "Attempting websocket upgrade");

    bufferResponseAndCommonHeaders(ResponseCode::WebSocketProtocolHandshake);
    bufferLine("Upgrade: websocket");
    bufferLine("Connection: Upgrade");
    bufferLine("Sec-WebSocket-Accept: " + getAcceptKey(webSocketKey));
    bufferLine("");
    flush();

    if (_webSocketHandler) {
        _webSocketHandler->onConnect(this);
    }
    _state = HANDLING_HYBI_WEBSOCKET;
    return true;
}

bool Connection::parseRange(const std::string& rangeStr, Range& range) const {
    size_t minusPos = rangeStr.find('-');
    if (minusPos == std::string::npos) {
        LS_WARNING(_logger, "Bad range: '" << rangeStr << "'");
        return false;
    }
    if (minusPos == 0) {
        // A range like "-500" means 500 bytes from end of file to end.
        range.start = atoi(rangeStr.c_str());
        range.end = std::numeric_limits<long>::max();
        return true;
    } else {
        range.start = atoi(rangeStr.substr(0, minusPos).c_str());
        if (minusPos == rangeStr.size()-1) {
            range.end = std::numeric_limits<long>::max();
        } else {
            range.end = atoi(rangeStr.substr(minusPos + 1).c_str());
        }
        return true;
    }
    return false;
}

bool Connection::parseRanges(const std::string& range, std::list<Range>& ranges) const {
    static const std::string expectedPrefix = "bytes=";
    if (range.length() < expectedPrefix.length() || range.substr(0, expectedPrefix.length()) != expectedPrefix) {
        LS_WARNING(_logger, "Bad range request prefix: '" << range << "'");
        return false;
    }
    auto rangesText = split(range.substr(expectedPrefix.length()), ',');
    for (auto it = rangesText.begin(); it != rangesText.end(); ++it) {
        Range r;
        if (!parseRange(*it, r)) {
            return false;
        }
        ranges.push_back(r);
    }
    return !ranges.empty();
}

// Sends HTTP 200 or 206, content-length, and range info as needed. Returns the actual file ranges
// needing sending.
std::list<Connection::Range> Connection::processRangesForStaticData(const std::list<Range>& origRanges, long fileSize) {
    if (origRanges.empty()) {
        // Easy case: a non-range request.
        bufferResponseAndCommonHeaders(ResponseCode::Ok);
        bufferLine("Content-Length: " + toString(fileSize));
        return { Range { 0, fileSize - 1 } };
    }

    // Partial content request.
    bufferResponseAndCommonHeaders(ResponseCode::PartialContent);
    int contentLength = 0;
    std::ostringstream rangeLine;
    rangeLine << "Content-Range: bytes ";
    std::list<Range> sendRanges;
    for (auto rangeIter = origRanges.cbegin(); rangeIter != origRanges.cend(); ++rangeIter) {
        Range actualRange = *rangeIter;
        if (actualRange.start < 0) {
            actualRange.start += fileSize;
        }
        if (actualRange.start >= fileSize) {
            actualRange.start = fileSize - 1;
        }
        if (actualRange.end >= fileSize) {
            actualRange.end = fileSize - 1;
        }
        contentLength += actualRange.length();
        sendRanges.push_back(actualRange);
        rangeLine << actualRange.start << "-" << actualRange.end;
    }
    rangeLine << "/" << fileSize;
    bufferLine(rangeLine.str());
    bufferLine("Content-Length: " + toString(contentLength));
    return sendRanges;
}

bool Connection::sendStaticData() {
    // TODO: fold this into the handler way of doing things.
    std::string path = _server.getStaticPath() + getRequestUri();
    auto rangeHeader = getHeader("Range");
    // Trim any trailing queries.
    size_t queryPos = path.find('?');
    if (queryPos != path.npos) {
        path.resize(queryPos);
    }
    if (*path.rbegin() == '/') {
        path += "index.html";
    }
    RaiiFd input(path.c_str());
    struct stat stat;
    if (!input.ok() || ::fstat(input, &stat) == -1) {
        return send404();
    }
    std::list<Range> ranges;
    if (!rangeHeader.empty() && !parseRanges(rangeHeader, ranges)) {
        return sendBadRequest("Bad range header");
    }
    ranges = processRangesForStaticData(ranges, stat.st_size);
    bufferLine("Content-Type: " + getContentType(path));
    bufferLine("Connection: keep-alive");
    bufferLine("Accept-Ranges: bytes");
    bufferLine("Last-Modified: " + webtime(stat.st_mtime));
    if (!isCacheable(path)) {
        bufferLine("Cache-Control: no-store");
        bufferLine("Pragma: no-cache");
        bufferLine("Expires: " + now());
    }
    bufferLine("");
    if (!flush()) {
        return false;
    }

    for (auto rangeIter = ranges.cbegin(); rangeIter != ranges.cend(); ++rangeIter) {
        if (::lseek(input, rangeIter->start, SEEK_SET) == -1) {
            // We've (probably) already sent data.
            return false;
        }
        auto bytesLeft = rangeIter->length();
        while (bytesLeft) {
            char buf[ReadWriteBufferSize];
            auto bytesRead = ::read(input, buf, std::min(sizeof(buf), bytesLeft));
            if (bytesRead <= 0) {
                const static std::string unexpectedEof("Unexpected EOF");
                LS_ERROR(_logger, "Error reading file: " << (bytesRead == 0 ? unexpectedEof : getLastError()));
                // We can't send an error document as we've sent the header.
                return false;
            }
            bytesLeft -= bytesRead;
            if (!write(buf, bytesRead, true)) {
                return false;
            }
        }
    }
    return true;
}

bool Connection::sendData(const std::string& type, const char* start, size_t size) {
    bufferResponseAndCommonHeaders(ResponseCode::Ok);
    bufferLine("Content-Type: " + type);
    bufferLine("Content-Length: " + toString(size));
    bufferLine("Connection: keep-alive");
    bufferLine("");
    bool result = write(start, size, true);
    return result;
}

void Connection::bufferResponseAndCommonHeaders(ResponseCode code) {
    auto responseCodeInt = static_cast<int>(code);
    auto responseCodeName = ::name(code);
    auto response = std::string("HTTP/1.1 " + toString(responseCodeInt) + " " + responseCodeName);
    LS_ACCESS(_logger, "Response: " << response);
    bufferLine(response);
    bufferLine("Server: " SEASOCKS_VERSION_STRING);
    bufferLine("Date: " + now());
    bufferLine("Access-Control-Allow-Origin: *");
}

void Connection::setLinger() {
    if (_fd == -1) {
        return;
    }
    const int secondsToLinger = 1;
    struct linger linger = { true, secondsToLinger };
    if (::setsockopt(_fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
        LS_INFO(_logger, "Unable to set linger on socket");
    }
}

bool Connection::hasHeader(const std::string& header) const {
    return _request ? _request->hasHeader(header) : false;
}

std::string Connection::getHeader(const std::string& header) const {
    return _request ? _request->getHeader(header) : "";
}

const std::string& Connection::getRequestUri() const {
    static const std::string empty;
    return _request ? _request->getRequestUri() : empty;
}

}  // seasocks
