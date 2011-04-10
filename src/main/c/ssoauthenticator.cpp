#include "seasocks/ssoauthenticator.h"

#include <string.h>
#include <iostream>

namespace SeaSocks {

SsoAuthenticator::SsoAuthenticator(SsoOptions options) : _options(options) {
}

bool SsoAuthenticator::isBounceBackFromSsoServer(const char* requestUri) {
	bool prefixMatch = _options.returnPath.compare(0, _options.returnPath.length(), requestUri,  _options.returnPath.length()) == 0;
	if (prefixMatch) {
		char nextChar = requestUri[_options.returnPath.length()];
		return nextChar == '\0' || nextChar == '?';
	} else {
		return false;
	}
}

bool SsoAuthenticator::enabledForPath(const char* requestUri) {
	// TODO: Provide API hooks to customize this.
	return true;
}

bool SsoAuthenticator::validateSignature(const char* requestUri) {
	// TODO: Implement this.
	return true;
}

bool SsoAuthenticator::respondWithLocalCookieAndRedirectToOriginalPage() {
	return false;
}

bool SsoAuthenticator::respondWithInvalidSignatureError() {
	return false;
}

bool SsoAuthenticator::respondWithRedirectToAuthenticationServer() {
	return false;
}

void SsoAuthenticator::extractCredentialsFromLocalCookie(boost::shared_ptr<Credentials> target) {
}

bool SsoAuthenticator::requestExplicityForbidsDrwSsoRedirect() {
	return false;
}

}