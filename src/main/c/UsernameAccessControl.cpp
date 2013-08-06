#include "seasocks/SsoAuthenticator.h"
#include "seasocks/UsernameAccessControl.h"

namespace seasocks {

UsernameAccessControl::UsernameAccessControl(const std::set<std::string> users) : _users(users) {
}

bool UsernameAccessControl::requiresAuthentication(const Request& request) {
    return true;
}

bool UsernameAccessControl::hasAccess(const Request& request) {
    auto credentials = request.credentials();
    std::cout << "hello " << credentials->username << "@" << request.getRequestUri() << std::endl;
    if (!credentials->authenticated) {
        return false;
    }
    return _users.find(credentials->username) != _users.end();
}

}
