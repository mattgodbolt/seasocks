#ifndef USERNAMEACCESSCONTROL_H_
#define USERNAMEACCESSCONTROL_H_

#include "seasocks/AccessControl.h"

#include <set>
#include <string>

namespace SeaSocks {

class UsernameAccessControl : public AccessControl {
	std::set<std::string> _users;
public:
	UsernameAccessControl(const std::set<std::string> users);
	virtual bool requiresAuthentication(const char* requestUri);
	virtual bool hasAccess(boost::shared_ptr<Credentials> credentials, const char* requestUri);
};

}

#endif /* USERNAMEACCESSCONTROL_H_ */
