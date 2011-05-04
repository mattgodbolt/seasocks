#include "seasocks/UsernameAccessControl.h"

#include "seasocks/ssoauthenticator.h"

namespace SeaSocks {

UsernameAccessControl::UsernameAccessControl(const std::set<std::string> users) : _users(users) {
}

bool UsernameAccessControl::requiresAuthentication(const char* requestUri) {
	return true;
}

bool UsernameAccessControl::hasAccess(boost::shared_ptr<Credentials> credentials, const char* requestUri) {
	std::cout << "hello " << credentials->username << "@" << requestUri << std::endl;
	if (!credentials->authenticated) {
		return false;
	}
	return _users.find(credentials->username) != _users.end();
}

}
