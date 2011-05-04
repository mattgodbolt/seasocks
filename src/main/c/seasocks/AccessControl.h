#ifndef ACCESSCONTROL_H_
#define ACCESSCONTROL_H_

#include <boost/shared_ptr.hpp>

namespace SeaSocks {

struct Credentials;

/**
 * Interface used to control who has access to which resources when using
 * SSO.
 */
class AccessControl {
public:
	virtual ~AccessControl() {}

	// Returns true to indicate a given URI requires login.
	virtual bool requiresAuthentication(const char* requestUri) = 0;

	// Returns true if this user has access to the given URI.
	virtual bool hasAccess(boost::shared_ptr<Credentials> credentials, const char* requestUri) = 0;
};

}

#endif /* ACCESSCONTROL_H_ */
