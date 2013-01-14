#pragma once

#include "seasocks/Request.h"
#include "seasocks/Response.h"
#include "seasocks/util/CrackedUri.h"

#include <memory>

namespace seasocks {

class CrackedUriPageHandler {
public:
    virtual ~CrackedUriPageHandler() {}

    virtual std::shared_ptr<seasocks::Response> handle(const CrackedUri& uri, const seasocks::Request& request) = 0;
};

}
