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

#include "internal/Config.h"
#include "internal/Embedded.h"
#include "internal/HeaderMap.h"
#include "internal/HybiAccept.h"
#include "internal/HybiPacketDecoder.h"
#include "internal/LogStream.h"
#include "internal/PageRequest.h"
#include "internal/RaiiFd.h"

#include "md5/md5.h"

#include "seasocks/Connection.h"
#include "seasocks/Credentials.h"
#include "seasocks/Logger.h"
#include "seasocks/PageHandler.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/ToString.h"
#include "seasocks/ResponseWriter.h"
#include "seasocks/ZlibContext.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <byteswap.h>
#include <unordered_map>
#include <memory>

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

char* extractLine(uint8_t*& first, uint8_t* last, char** colon = nullptr) {
    for (uint8_t* ptr = first; ptr < last - 1; ++ptr) {
        if (ptr[0] == '\r' && ptr[1] == '\n') {
            ptr[0] = 0;
            uint8_t* result = first;
            first = ptr + 2;
            return reinterpret_cast<char*>(result);
        }
        if (colon && ptr[0] == ':' && *colon == nullptr) {
            *colon = reinterpret_cast<char*>(ptr);
        }
    }
    return nullptr;
}

const std::unordered_map<std::string, std::string> contentTypes = {
    {"txt", "text/plain"},
    {"css", "text/css"},
    {"csv", "text/csv"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"xml", "text/xml"},
    {"js", "text/javascript"}, // Technically it should be application/javascript (RFC 4329), but IE8 struggles with that
    {"xhtml", "application/xhtml+xml"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"tar", "application/x-tar"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"tiff", "image/tiff"},
    {"tif", "image/tiff"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"ico", "image/x-icon"},
    {"swf", "application/x-shockwave-flash"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/x-wav"},
    {"ttf", "font/ttf"},
};

std::string getExt(const std::string& path) {
    auto lastDot = path.find_last_of('.');
    if (lastDot != std::string::npos) {
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

constexpr size_t ReadWriteBufferSize = 16 * 1024;
constexpr size_t MaxWebsocketMessageSize = 16384;
constexpr size_t MaxHeadersSize = 64 * 1024;

class PrefixWrapper : public seasocks::Logger {
    std::string _prefix;
    std::shared_ptr<Logger> _logger;

public:
    PrefixWrapper(const std::string& prefix, std::shared_ptr<Logger> logger)
            : _prefix(prefix), _logger(logger) {
    }

    void log(Level level, const char* message) override {
        _logger->log(level, (_prefix + message).c_str());
    }
};

bool hasConnectionType(const std::string& connection, const std::string& type) {
    for (auto conType : seasocks::split(connection, ',')) {
        while (!conType.empty() && isspace(conType[0]))
            conType = conType.substr(1);
        if (seasocks::caseInsensitiveSame(conType, type))
            return true;
    }
    return false;
}

} // namespace

namespace seasocks {

struct Connection::Writer : ResponseWriter {
    Connection* _connection;
    explicit Writer(Connection& connection)
            : _connection(&connection) {
    }

    void detach() {
        _connection = nullptr;
    }

    void begin(ResponseCode responseCode, TransferEncoding encoding) override {
        if (_connection)
            _connection->begin(responseCode, encoding);
    }
    void header(const std::string& header, const std::string& value) override {
        if (_connection)
            _connection->header(header, value);
    }
    void payload(const void* data, size_t size, bool flush) override {
        if (_connection)
            _connection->payload(data, size, flush);
    }
    void finish(bool keepConnectionOpen) override {
        if (_connection)
            _connection->finish(keepConnectionOpen);
    }
    void error(ResponseCode responseCode, const std::string& payload) override {
        if (_connection)
            _connection->error(responseCode, payload);
    }

    bool isActive() const override {
        return _connection;
    }
};

Connection::Connection(
    std::shared_ptr<Logger> logger,
    ServerImpl& server,
    int fd,
    const sockaddr_in& address)
        : _logger(std::make_shared<PrefixWrapper>(formatAddress(address) + " : ", logger)),
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
          _transferEncoding(TransferEncoding::Raw),
          _chunk(0u),
          _writer(std::make_shared<Writer>(*this)),
          _state(State::READING_HEADERS) {
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
    if (_response) {
        _response->cancel();
        _response.reset();
        _writer->detach();
        _writer.reset();
    }
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

ssize_t Connection::safeSend(const void* data, size_t size) {
    if (_fd == -1 || _hadSendError || _shutdown) {
        // Ignore further writes to the socket, it's already closed or has been shutdown
        return -1;
    }
    auto sendResult = ::send(_fd, data, size, MSG_NOSIGNAL);
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
        ssize_t bytesSent = 0;
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
        if (newBufferSize >= _server.clientBufferSize()) {
            LS_WARNING(_logger, "Closing connection: buffer size too large ("
                                    << newBufferSize << " >= " << _server.clientBufferSize() << ")");
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
    static const char crlf[] = {'\r', '\n'};
    if (!write(line, strlen(line), false))
        return false;
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
    auto result = ::read(_fd, &_inBuf[curSize], ReadWriteBufferSize);
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
    auto numSent = safeSend(&_outBuf[0], _outBuf.size());
    if (numSent == -1) {
        return false;
    }
    _outBuf.erase(_outBuf.begin(), _outBuf.begin() + numSent);
    if (!_outBuf.empty() && !_registeredForWriteEvents) {
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
        case State::READING_HEADERS:
            handleHeaders();
            break;
        case State::READING_WEBSOCKET_KEY3:
            handleWebSocketKey3();
            break;
        case State::HANDLING_HIXIE_WEBSOCKET:
            handleHixieWebSocket();
            break;
        case State::HANDLING_HYBI_WEBSOCKET:
            handleHybiWebSocket();
            break;
        case State::BUFFERING_POST_DATA:
            handleBufferingPostData();
            break;
        case State::AWAITING_RESPONSE_BEGIN:
        case State::SENDING_RESPONSE_BODY:
        case State::SENDING_RESPONSE_HEADERS:
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
            _inBuf[i + 1] == '\n' &&
            _inBuf[i + 2] == '\r' &&
            _inBuf[i + 3] == '\n') {
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
        bufferLine("Sec-WebSocket-Origin: " + _request->getHeader("Origin"));
    }
    if (_request->hasHeader("Host")) {
        auto host = _request->getHeader("Host");
        if (!allowCrossOrigin) {
            bufferLine("Sec-WebSocket-Origin: http://" + host);
        }
        bufferLine("Sec-WebSocket-Location: ws://" + host + _request->getRequestUri());
    }
    pickProtocol();
    bufferLine("");

    write(&digest, 16, true);

    _state = State::HANDLING_HIXIE_WEBSOCKET;
    _inBuf.erase(_inBuf.begin(), _inBuf.begin() + 8);
    if (_webSocketHandler) {
        _webSocketHandler->onConnect(this);
    }
}

void Connection::pickProtocol() {
    static std::string protocolHeader = "Sec-WebSocket-Protocol";
    if (!_request->hasHeader(protocolHeader) || !_webSocketHandler)
        return;
    // Ideally we need o support this header being set multiple times...but the headers don't support that.
    auto protocols = split(_request->getHeader(protocolHeader), ',');
    LS_DEBUG(_logger, "Requested protocols:");
    std::transform(protocols.begin(), protocols.end(), protocols.begin(), trimWhitespace);
    for (auto&& p : protocols) {
        LS_DEBUG(_logger, "  " + p);
    }
    auto choice = _webSocketHandler->chooseProtocol(protocols);
    if (choice >= 0 && choice < static_cast<ssize_t>(protocols.size())) {
        LS_DEBUG(_logger, "Chose protocol " + protocols[choice]);
        bufferLine(protocolHeader + ": " + protocols[choice]);
    }
}

void Connection::handleBufferingPostData() {
    if (_request->consumeContent(_inBuf)) {
        _state = State::READING_HEADERS;
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
    if (_state == State::HANDLING_HIXIE_WEBSOCKET) {
        uint8_t zero = 0;
        if (!write(&zero, 1, false))
            return;
        if (!write(webSocketResponse, messageLength, false))
            return;
        uint8_t effeff = 0xff;
        write(&effeff, 1, true);
        return;
    }
    sendHybi(static_cast<uint8_t>(HybiPacketDecoder::Opcode::Text),
             reinterpret_cast<const uint8_t*>(webSocketResponse), messageLength);
}

void Connection::send(const uint8_t* webSocketResponse, size_t length) {
    _server.checkThread();
    if (_shutdown) {
        if (_shutdownByUser) {
            LS_ERROR(_logger, "Client wrote to connection after closing it");
        }
        return;
    }
    if (_state == State::HANDLING_HIXIE_WEBSOCKET) {
        LS_ERROR(_logger, "Hixie does not support binary");
        return;
    }
    sendHybi(static_cast<uint8_t>(HybiPacketDecoder::Opcode::Binary), webSocketResponse, length);
}

void Connection::sendHybi(uint8_t opcode, const uint8_t* webSocketResponse, size_t messageLength) {
    uint8_t firstByte = 0x80 | opcode;
    if (_perMessageDeflate)
        firstByte |= 0x40;
    if (!write(&firstByte, 1, false))
        return;

    if (_perMessageDeflate) {
        std::vector<uint8_t> compressed;

        zlibContext.deflate(webSocketResponse, messageLength, compressed);

        LS_DEBUG(_logger, "Compression result: " << messageLength << " bytes -> " << compressed.size() << " bytes");
        sendHybiData(compressed.data(), compressed.size());
    } else {
        sendHybiData(webSocketResponse, messageLength);
    }
}

void Connection::sendHybiData(const uint8_t* webSocketResponse, size_t messageLength) {
    if (messageLength < 126) {
        uint8_t nextByte = messageLength; // No MASK bit set.
        if (!write(&nextByte, 1, false))
            return;
    } else if (messageLength < 65536) {
        uint8_t nextByte = 126; // No MASK bit set.
        if (!write(&nextByte, 1, false))
            return;
        auto lengthBytes = htons(messageLength);
        if (!write(&lengthBytes, 2, false))
            return;
    } else {
        uint8_t nextByte = 127; // No MASK bit set.
        if (!write(&nextByte, 1, false))
            return;
        uint64_t lengthBytes = __bswap_64(messageLength);
        if (!write(&lengthBytes, 8, false))
            return;
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
            LS_WARNING(_logger, "Error in WebSocket input stream (got " << (int) _inBuf[messageStart] << ")");
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
        bool deflateNeeded = false;

        auto messageState = decoder.decodeNextMessage(decodedMessage, deflateNeeded);

        if (deflateNeeded) {
            if (!_perMessageDeflate) {
                LS_WARNING(_logger, "Received deflated hybi frame but deflate wasn't negotiated");
                closeInternal();
                return;
            }

            size_t compressed_size = decodedMessage.size();

            std::vector<uint8_t> decompressed;
            int zlibError;

            // Note: inflate() alters decodedMessage
            bool success = zlibContext.inflate(decodedMessage, decompressed, zlibError);

            if (!success) {
                LS_WARNING(_logger, "Decompression error from zlib: " << zlibError);
                closeInternal();
                return;
            }

            LS_DEBUG(_logger, "Decompression result: " << compressed_size << " bytes -> " << decodedMessage.size() << " bytes");

            decodedMessage.swap(decompressed);
        }


        switch (messageState) {
            default:
                closeInternal();
                LS_WARNING(_logger, "Unknown HybiPacketDecoder state");
                return;
            case HybiPacketDecoder::MessageState::Error:
                closeInternal();
                return;
            case HybiPacketDecoder::MessageState::TextMessage:
                decodedMessage.push_back(0); // avoids a copy
                handleWebSocketTextMessage(reinterpret_cast<const char*>(&decodedMessage[0]));
                break;
            case HybiPacketDecoder::MessageState::BinaryMessage:
                handleWebSocketBinaryMessage(decodedMessage);
                break;
            case HybiPacketDecoder::MessageState::Ping:
                sendHybi(static_cast<uint8_t>(HybiPacketDecoder::Opcode::Pong),
                         &decodedMessage[0], decodedMessage.size());
                break;
            case HybiPacketDecoder::MessageState::Pong:
                // Pongs can be sent unsolicited (MSIE and Edge do this)
                // The spec says to ignore them.
                break;
            case HybiPacketDecoder::MessageState::NoMessage:
                done = true;
                break;
            case HybiPacketDecoder::MessageState::Close:
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
    assert(_state != State::HANDLING_HIXIE_WEBSOCKET);
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
                    << "<div>" << body << "</div><hr/><div><i>Powered by "
                                          "<a href=\"https://github.com/mattgodbolt/seasocks\">Seasocks</a></i></div></body></html>";
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
    assert(requestLine != nullptr);

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
    if (requestUri == nullptr) {
        return sendBadRequest("Malformed request line");
    }

    const char* httpVersion = shift(requestLine);
    if (httpVersion == nullptr) {
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
        char* colonPos = nullptr;
        char* headerLine = extractLine(first, last, &colonPos);
        assert(headerLine != nullptr);
        if (colonPos == nullptr) {
            return sendBadRequest("Malformed header");
        }
        *colonPos = 0;
        const char* key = headerLine;
        const char* value = skipWhitespace(colonPos + 1);
        LS_DEBUG(_logger, "Key: " << key << " || " << value);
        headers.emplace(key, value);
    }

    if (headers.count("Connection") && headers.count("Upgrade") && hasConnectionType(headers["Connection"], "Upgrade") && caseInsensitiveSame(headers["Upgrade"], "websocket")) {
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

        if (_server.server().getPerMessageDeflateEnabled() && headers.count("Sec-WebSocket-Extensions")) {
            parsePerMessageDeflateHeader(headers["Sec-WebSocket-Extensions"]);
        }
    }

    _request = std::make_unique<PageRequest>(_address, requestUri, _server.server(),
                                             verb, std::move(headers));

    const EmbeddedContent* embedded = findEmbeddedContent(requestUri);
    if (verb == Request::Verb::Get && embedded) {
        // MRG: one day, this could be a request handler.
        return sendData(getContentType(requestUri), embedded->data, embedded->length);
    } else if (verb == Request::Verb::Head && embedded) {
        return sendHeader(getContentType(requestUri), embedded->length);
    }

    if (_request->contentLength() > _server.clientBufferSize()) {
        return sendBadRequest("Content length too long");
    }
    if (_request->contentLength() == 0) {
        return handlePageRequest();
    }
    _state = State::BUFFERING_POST_DATA;
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
        int webSocketVersion{0};
        try {
            webSocketVersion = std::stoi(_request->getHeader("Sec-WebSocket-Version"));
        } catch (const std::logic_error& ex) {
            LS_WARNING(_logger, "Invalid Sec-WebSocket-Version '" << _request->getHeader("Sec-WebSocket-Version") << "'");
            return sendError(ResponseCode::UpgradeRequired, "Invalid Sec-WebSocket-Version received");
        }
        if (!_webSocketHandler) {
            LS_WARNING(_logger, "Couldn't find WebSocket end point for '" << uri << "'");
            return send404();
        }
        if (webSocketVersion == 0) {
            // Hixie
            _state = State::READING_WEBSOCKET_KEY3;
            return true;
        }
        auto hybiKey = _request->getHeader("Sec-WebSocket-Key");
        return handleHybiHandshake(webSocketVersion, hybiKey);
    }
    return sendResponse(response);
}

bool Connection::sendResponse(std::shared_ptr<Response> response) {
    if (response == Response::unhandled()) {
        return sendStaticData();
    }
    assert(_response.get() == nullptr);
    _state = State::AWAITING_RESPONSE_BEGIN;
    _transferEncoding = TransferEncoding::Raw;
    _chunk = 0;
    _response = response;
    _response->handle(_writer);
    return true;
}

void Connection::error(ResponseCode responseCode, const std::string& payload) {
    _server.checkThread();
    if (_state != State::AWAITING_RESPONSE_BEGIN) {
        LS_ERROR(_logger, "error() called when in wrong state");
        return;
    }
    if (isOk(responseCode)) {
        LS_ERROR(_logger, "error() called with a non-error code");
    }
    if (responseCode == ResponseCode::NotFound) {
        // TODO: better here; we use this purely to serve our own embedded content.
        send404();
    } else {
        sendError(responseCode, payload);
    }
}

void Connection::begin(ResponseCode responseCode, TransferEncoding encoding) {
    _server.checkThread();
    if (_state != State::AWAITING_RESPONSE_BEGIN) {
        LS_ERROR(_logger, "begin() called when in wrong state");
        return;
    }
    _state = State::SENDING_RESPONSE_HEADERS;
    bufferResponseAndCommonHeaders(responseCode);
    _transferEncoding = encoding;
    if (_transferEncoding == TransferEncoding::Chunked) {
        bufferLine("Transfer-encoding: chunked");
    }
}

void Connection::header(const std::string& header, const std::string& value) {
    _server.checkThread();
    if (_state != State::SENDING_RESPONSE_HEADERS) {
        LS_ERROR(_logger, "header() called when in wrong state");
        return;
    }
    bufferLine(header + ": " + value);
}
void Connection::payload(const void* data, size_t size, bool flush) {
    _server.checkThread();
    if (_state == State::SENDING_RESPONSE_HEADERS) {
        bufferLine("");
        _state = State::SENDING_RESPONSE_BODY;
    } else if (_state != State::SENDING_RESPONSE_BODY) {
        LS_ERROR(_logger, "payload() called when in wrong state");
        return;
    }
    if (size && _transferEncoding == TransferEncoding::Chunked) {
        writeChunkHeader(size);
    }
    write(data, size, flush);
}

void Connection::writeChunkHeader(size_t size) {
    std::ostringstream lengthStr;
    if (_chunk)
        lengthStr << "\r\n";
    lengthStr << std::hex << size << "\r\n";
    auto length = lengthStr.str();
    _chunk++;
    write(length.c_str(), length.size(), false);
}

void Connection::finish(bool keepConnectionOpen) {
    _server.checkThread();
    if (_state == State::SENDING_RESPONSE_HEADERS) {
        bufferLine("");
    } else if (_state != State::SENDING_RESPONSE_BODY) {
        LS_ERROR(_logger, "finish() called when in wrong state");
        return;
    }
    if (_transferEncoding == TransferEncoding::Chunked) {
        writeChunkHeader(0);
        write("\r\n", 2, false);
    }

    flush();

    if (!keepConnectionOpen) {
        closeWhenEmpty();
    }

    _state = State::READING_HEADERS;
    _response.reset();
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
    if (_perMessageDeflate)
        bufferLine("Sec-WebSocket-Extensions: permessage-deflate");
    pickProtocol();
    bufferLine("");
    flush();

    if (_webSocketHandler) {
        _webSocketHandler->onConnect(this);
    }
    _state = State::HANDLING_HYBI_WEBSOCKET;
    return true;
}

void Connection::parsePerMessageDeflateHeader(const std::string& header) {
    for (auto& extField : seasocks::split(header, ';')) {
        while (!extField.empty() && isspace(extField[0])) {
            extField = extField.substr(1);
        }

        if (seasocks::caseInsensitiveSame(extField, "permessage-deflate")) {
            LS_INFO(_logger, "Enabling per-message deflate");
            _perMessageDeflate = true;
            zlibContext.initialise();
        }
    }
}

bool Connection::parseRange(const std::string& rangeStr, Range& range) const {
    size_t minusPos = rangeStr.find('-');
    if (minusPos == std::string::npos) {
        LS_WARNING(_logger, "Bad range: '" << rangeStr << "'");
        return false;
    }
    if (minusPos == 0) {
        // A range like "-500" means 500 bytes from end of file to end.
        range.start = std::stoi(rangeStr);
        range.end = std::numeric_limits<long>::max();
        return true;
    } else {
        range.start = std::stoi(rangeStr.substr(0, minusPos));
        if (minusPos == rangeStr.size() - 1) {
            range.end = std::numeric_limits<long>::max();
        } else {
            range.end = std::stoi(rangeStr.substr(minusPos + 1));
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
    for (auto& it : rangesText) {
        Range r;
        if (!parseRange(it, r)) {
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
        return {Range{0, fileSize - 1}};
    }

    // Partial content request.
    bufferResponseAndCommonHeaders(ResponseCode::PartialContent);
    int contentLength = 0;
    std::ostringstream rangeLine;
    rangeLine << "Content-Range: bytes ";
    std::list<Range> sendRanges;
    for (auto actualRange : origRanges) {
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
    if (queryPos != std::string::npos) {
        path.resize(queryPos);
    }
    if (*path.rbegin() == '/') {
        path += "index.html";
    }

    RaiiFd input{::open(path.c_str(), O_RDONLY)};
    struct stat fileStat;
    if (!input.ok() || ::fstat(input, &fileStat) == -1) {
        return send404();
    }
    std::list<Range> ranges;
    if (!rangeHeader.empty() && !parseRanges(rangeHeader, ranges)) {
        return sendBadRequest("Bad range header");
    }
    ranges = processRangesForStaticData(ranges, fileStat.st_size);
    bufferLine("Content-Type: " + getContentType(path));
    bufferLine("Connection: keep-alive");
    bufferLine("Accept-Ranges: bytes");
    bufferLine("Last-Modified: " + webtime(fileStat.st_mtime));
    if (!isCacheable(path)) {
        bufferLine("Cache-Control: no-store");
        bufferLine("Pragma: no-cache");
        bufferLine("Expires: " + now());
    }
    bufferLine("");
    if (!flush()) {
        return false;
    }

    for (auto range : ranges) {
        if (::lseek(input, range.start, SEEK_SET) == -1) {
            // We've (probably) already sent data.
            return false;
        }
        auto bytesLeft = range.length();
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

bool Connection::sendHeader(const std::string& type, size_t size) {
    bufferResponseAndCommonHeaders(ResponseCode::Ok);
    bufferLine("Content-Type: " + type);
    bufferLine("Content-Length: " + toString(size));
    bufferLine("Connection: keep-alive");
    return bufferLine("");
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
    bufferLine("Server: " + std::string(Config::version));
    bufferLine("Date: " + now());
    bufferLine("Access-Control-Allow-Origin: *");
}

void Connection::setLinger() {
    if (_fd == -1) {
        return;
    }
    const int secondsToLinger = 1;
    struct linger linger = {true, secondsToLinger};
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

Server& Connection::server() const {
    return _server.server();
}

} // seasocks
