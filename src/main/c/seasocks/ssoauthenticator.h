#ifndef _SEASOCKS_SSO_AUTHENTICATOR_H_
#define _SEASOCKS_SSO_AUTHENTICATOR_H_

#include "seasocks/credentials.h"

#include <boost/shared_ptr.hpp>

namespace SeaSocks {

// TODO: Implement this!
	
class SsoAuthenticator {
public:

	bool isBounceBackFromSsoServer(const char* requestUri) {
		return false;
	}

	bool validateSignature(const char* requestUri) {
		return false;
	}

	bool respondWithLocalCookieAndRedirectToOriginalPage() {
		return false;
	}

	bool respondWithInvalidSignatureError() {
		return false;
	}

	bool respondWithRedirectToAuthenticationServer() {
		return false;
	}

	void extractCredentialsFromLocalCookie(boost::shared_ptr<Credentials> target) {
	}
	
	bool requestExplicityForbidsDrwSsoRedirect() {
		return false;
	}

};

}  // namespace SeaSocks

#endif  // _SEASOCKS_SSO_AUTHENTICATOR_H_
