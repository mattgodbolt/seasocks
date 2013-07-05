#include <seasocks/util/CrackedUriPageHandler.h>
#include <seasocks/Response.h>

#include <memory>

namespace seasocks {

// Returns a canned response for a given pathname.
class StaticResponseHandler : public CrackedUriPageHandler {
    std::string _path;
    std::shared_ptr<Response> _response;

public:
    StaticResponseHandler(const std::string& path, std::shared_ptr<Response> response)
    : _path(path), _response(response) {}

    virtual std::shared_ptr<Response> handle(
            const CrackedUri& uri,
            const Request&) override {
        if (uri.path().size() != 1 || uri.path()[0] != _path)
            return Response::unhandled();
        return _response;
    }
};

}
