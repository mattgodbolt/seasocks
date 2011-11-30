#include "seasocks/ResponseBuilder.h"

#include "internal/ConcreteResponse.h"

namespace SeaSocks {

ResponseBuilder::ResponseBuilder(ResponseCode code) :
        _code(code),
        _contentType("text/plain"),
        _keepAlive(true),
        _stream(new std::ostringstream) {

}

ResponseBuilder& ResponseBuilder::asHtml() {
    return withContentType("text/html");
}

ResponseBuilder& ResponseBuilder::asText() {
    return withContentType("text/plain");
}

ResponseBuilder& ResponseBuilder::asJson() {
    return withContentType("application/json");
}

ResponseBuilder& ResponseBuilder::withContentType(const std::string& contentType) {
    _contentType = contentType;
    return *this;
}

ResponseBuilder& ResponseBuilder::keepsConnectionAlive() {
    _keepAlive = true;
    return *this;
}

ResponseBuilder& ResponseBuilder::closesConnection() {
    _keepAlive = false;
    return *this;
}

ResponseBuilder& ResponseBuilder::withLocation(const std::string& location) {
    return withHeader("Location", location);
}

ResponseBuilder& ResponseBuilder::setsCookie(const std::string& cookie, const std::string& value) {
    return addHeader("Set-Cookie", cookie + "=" + value);
}

ResponseBuilder& ResponseBuilder::withHeader(const std::string& name, const std::string& value) {
    _headers.erase(name);
    _headers.insert(std::make_pair(name, value));
    return *this;
}

ResponseBuilder& ResponseBuilder::addHeader(const std::string& name, const std::string& value) {
    _headers.insert(std::make_pair(name, value));
    return *this;
}

boost::shared_ptr<Response> ResponseBuilder::build() {
    return boost::shared_ptr<Response>(new ConcreteResponse(_code, _stream->str(), _contentType, _headers, _keepAlive));
}

}
