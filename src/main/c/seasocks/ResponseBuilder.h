#pragma once

#include <seasocks/Response.h>

#include <string>
#include <sstream>

namespace SeaSocks {

class ResponseBuilder {
    ResponseCode _code;
    std::string _contentType;
    bool _keepAlive;
    Response::Headers _headers;
    std::shared_ptr<std::ostringstream> _stream;
public:
    ResponseBuilder(ResponseCode code = ResponseCode::Ok);

    ResponseBuilder& asHtml();
    ResponseBuilder& asText();
    ResponseBuilder& asJson();
    ResponseBuilder& withContentType(const std::string& contentType);

    ResponseBuilder& keepsConnectionAlive();
    ResponseBuilder& closesConnection();

    ResponseBuilder& withLocation(const std::string& location);

    ResponseBuilder& setsCookie(const std::string& cookie, const std::string& value);

    ResponseBuilder& withHeader(const std::string& name, const std::string& value);
    template<typename T>
    ResponseBuilder& withHeader(const std::string& name, const T& t) {
        return withHeader(name, toString(t));
    }

    ResponseBuilder& addHeader(const std::string& name, const std::string& value);
    template<typename T>
    ResponseBuilder& addHeader(const std::string& name, const T& t) {
        return addHeader(name, toString(t));
    }

    template<typename T>
    ResponseBuilder& operator << (T&& t) {
        (*_stream) << std::forward(t);
        return *this;
    }

    std::shared_ptr<Response> build();
};

}
