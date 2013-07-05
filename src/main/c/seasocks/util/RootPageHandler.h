#pragma once

#include "seasocks/PageHandler.h"
#include "seasocks/util/CrackedUriPageHandler.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace seasocks {

class RootPageHandler : public PageHandler {
    std::unordered_map<std::string, std::shared_ptr<CrackedUriPageHandler>> _handlers;

public:
    RootPageHandler();
    virtual ~RootPageHandler();

    void add(const std::string& url, const std::shared_ptr<CrackedUriPageHandler>& handler);

    // From PageHandler.
    virtual std::shared_ptr<Response> handle(const Request& request) override;
};

}
