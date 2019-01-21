// Copyright (c) 2013-2017, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
    PathHandler(const std::string& path)
            : _path(path), _handlers() {
    }
    template <typename... Args>
    PathHandler(const std::string& path, Args&&... args)
            : _path(path), _handlers() {
        addMany(std::forward<Args>(args)...);
    }

    CrackedUriPageHandler::Ptr add(const CrackedUriPageHandler::Ptr& handler);

    void addMany() {
    }
    void addMany(const CrackedUriPageHandler::Ptr& handler) {
        add(handler);
    }
    template <typename... Rest>
    void addMany(const CrackedUriPageHandler::Ptr& handler, Rest&&... rest) {
        add(handler);
        addMany(std::forward<Rest>(rest)...);
    }

    virtual std::shared_ptr<Response> handle(
        const CrackedUri& uri, const Request& request) override;
};

}
