#pragma once

#include "seasocks/PageHandler.h"
#include "seasocks/util/CrackedUriPageHandler.h"

#include <memory>
#include <vector>

namespace seasocks {

class RootPageHandler : public PageHandler {
    std::vector<CrackedUriPageHandler::Ptr> _handlers;

public:
    RootPageHandler();
    virtual ~RootPageHandler();

    CrackedUriPageHandler::Ptr add(const CrackedUriPageHandler::Ptr& handler);

    // From PageHandler.
    virtual std::shared_ptr<Response> handle(const Request& request) override;
};

}
