#pragma once

#include "seasocks/ResponseCode.h"

#include <map>
#include <memory>
#include <string>

namespace seasocks {

class Response {
public:
    virtual ~Response() {}
    virtual ResponseCode responseCode() const = 0;

    virtual const char* payload() const = 0;
    virtual size_t payloadSize() const = 0;

    virtual std::string contentType() const = 0;

    virtual bool keepConnectionAlive() const = 0;

    typedef std::multimap<std::string, std::string> Headers;
    virtual Headers getAdditionalHeaders() const = 0;

    static std::shared_ptr<Response> unhandled();

    static std::shared_ptr<Response> notFound();

    static std::shared_ptr<Response> error(ResponseCode code, const std::string& error);

    static std::shared_ptr<Response> textResponse(const std::string& response);
    static std::shared_ptr<Response> jsonResponse(const std::string& response);
    static std::shared_ptr<Response> htmlResponse(const std::string& response);
};

}
