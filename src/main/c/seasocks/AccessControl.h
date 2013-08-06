#pragma once

#include <memory>

namespace seasocks {

struct Credentials;
class Request;

/**
 * Interface used to control who has access to which resources when using
 * SSO.
 */
class AccessControl {
public:
    virtual ~AccessControl() {}

    // Returns true to indicate a given request requires login.
    virtual bool requiresAuthentication(const Request& request) = 0;

    // Returns true if this request can be satisfied for the request's credentials.
    virtual bool hasAccess(const Request& request) = 0;
};

}
