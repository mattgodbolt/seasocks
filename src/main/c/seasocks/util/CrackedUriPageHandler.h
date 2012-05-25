#pragma once

#include "seasocks/Request.h"
#include "seasocks/Response.h"

#include "seasocks/util/CrackedUri.h"

#include <boost/shared_ptr.hpp>

namespace SeaSocks {

class CrackedUriPageHandler {
public:
    virtual ~CrackedUriPageHandler() {}

    virtual boost::shared_ptr<SeaSocks::Response> handle(const CrackedUri& uri, const SeaSocks::Request& request) = 0;
};

}
