#pragma once

#include "seasocks/util/CrackedUriPageHandler.h"

#include <string>

namespace seasocks {

// Handles a subpath of a website.  Passes on a shifted path to the registered
// subhandlers.  Useful, for example, to place a bunch of handlers beneath a
// common subpath on the server, e.g.
// PathHandler("debug")
//   .add(make_shared<DebugStatsHandler>())
//   .add(make_shared<AnotherDebugThing>())
//   .add(...) etc
// Each handler's CrackedUri will be shifted (i.e. won't have the "debug"
// prefix)
class PathHandler : public CrackedUriPageHandler {
    std::string _path;
    std::vector<CrackedUriPageHandler::Ptr> _handlers;

public:
    PathHandler(const std::string &path) : _path(path) {}
    template<typename... Args>
    PathHandler(const std::string &path, Args&&... args) : _path(path) {
        addMany(std::forward<Args>(args)...);
    }

    CrackedUriPageHandler::Ptr add(const CrackedUriPageHandler::Ptr& handler);

    void addMany() {}
    void addMany(const CrackedUriPageHandler::Ptr& handler) { add(handler); }
    template<typename... Rest>
    void addMany(const CrackedUriPageHandler::Ptr& handler, Rest&&... rest) {
        add(handler);
        addMany(std::forward<Rest>(rest)...);
    }

    virtual std::shared_ptr<Response> handle(
            const CrackedUri& uri, const Request& request) override;
};

}
