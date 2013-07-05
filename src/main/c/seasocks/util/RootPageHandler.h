#pragma once

#include "seasocks/PageHandler.h"
#include "seasocks/util/CrackedUriPageHandler.h"

#include <memory>
#include <vector>

namespace seasocks {

class RootPageHandler : public PageHandler {
    std::vector<std::shared_ptr<CrackedUriPageHandler>> _handlers;

public:
    RootPageHandler();
    virtual ~RootPageHandler();

    void add(const std::shared_ptr<CrackedUriPageHandler>& handler);

    // From PageHandler.
    virtual std::shared_ptr<Response> handle(const Request& request) override;
};

}
