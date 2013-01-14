#pragma once

#include "seasocks/AccessControl.h"

#include <set>
#include <string>

namespace seasocks {

class UsernameAccessControl : public AccessControl {
	std::set<std::string> _users;
public:
	UsernameAccessControl(const std::set<std::string> users);
	virtual bool requiresAuthentication(const Request& request);
	virtual bool hasAccess(const Request& request);
};

}
