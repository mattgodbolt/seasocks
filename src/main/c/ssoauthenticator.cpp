#include "seasocks/ssoauthenticator.h"

#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

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

bool SsoAuthenticator::respondWithLocalCookieAndRedirectToOriginalPage(const char* requestUri, std::ostream& response, std::string& error) {
	std::map<std::string, std::string> params;
	parseUriParameters(requestUri, params);
	if (params.count("user") == 0 || params.count("continue") == 0) {
		// Tampering detected.
		error = "Invalid response from SSO server (require user and continue params)";
		return false;
	}
	std::string user = params["user"];
	std::string continueUrl = params["continue"];
	if (user.empty() || continueUrl.empty()) {
		// Tampering detected.
		error = "Invalid response from SSO server (require user and continue param values)";
		return false;
	}
	if (user.find("\r") != std::string::npos
	    || user.find("\n") != std::string::npos
	    || continueUrl.find("://") != std::string::npos
	    || continueUrl.find("\r") != std::string::npos
	    || continueUrl.find("\n") != std::string::npos) {
		// Tampering detected.
		error = "Invalid response from SSO server (user or continue params contain invalid chars)";
		return false;
	}
	response << "HTTP/1.1 307 Temporary Redirect\r\n"
		 << "Location: " << continueUrl << "\r\n"
	         << "Set-Cookie: " << _options.authCookieName
		 << "=" << encodeUriComponent(user) << "|" << encodeUriComponent(secureHash(user)) << "\r\n"
		 << "\r\n";
	return true;
}

bool SsoAuthenticator::respondWithRedirectToAuthenticationServer(const char* requestUri, std::ostream& response, std::string& error) {
	return false;
}

void SsoAuthenticator::extractCredentialsFromLocalCookie(boost::shared_ptr<Credentials> target) {
}

bool SsoAuthenticator::requestExplicityForbidsDrwSsoRedirect() {
	return false;
}

std::string SsoAuthenticator::encodeUriComponent(const std::string& value) {
	std::stringstream result;
	for (int i = 0, l = value.length(); i < l; ++i) {
		char c = value[i];
		if (c == ' ') {
			result << '+';
		} else if (isalnum(c)) {
			result << c;
		} else {
			result << '%' << std::hex << std::uppercase << std::setw(2) << (int)c << std::nouppercase;
		}
	}
	return result.str();
}

std::string SsoAuthenticator::decodeUriComponent(const char* value, const char* end) {
	std::string result;
	char hexbuffer[3];
	hexbuffer[2] = '\0';
	while (value != end) {
		if (*value == '%' && value + 1 != end && value + 2 != end && isxdigit(*(value + 1)) && isxdigit(*(value + 2))) {
			value++;
			hexbuffer[0] = tolower(*(value++));
			hexbuffer[1] = tolower(*(value++));
			result += (char)strtol(hexbuffer, (char**) NULL, 16);
		} else if (*value == '+') {
			value++;
			result += ' ';
		} else {
			result += *(value++);
		}
	}
	return result;
}

void SsoAuthenticator::parseUriParameters(const char* uri, std::map<std::string, std::string>& params) {
	enum State {
		INITIAL, KEY, VALUE
	};
	State state = State::INITIAL;
	const char* keyStart = NULL;
	const char* keyEnd = NULL;
	const char* valueStart = NULL;
	const char* valueEnd = NULL;
	for (const char *pos = uri;; ++pos) {
		switch (*pos) {
		case '?':
			switch (state) {
			case State::INITIAL:
				state = State::KEY;
				keyStart = pos + 1;
				break;
			}
			break;
		case '&':
		case '\0':
			switch (state) {
			case State::KEY:
				break;
			case State::VALUE:
				valueEnd = pos;
				if (keyStart && keyEnd && valueStart) {
					std::string key = decodeUriComponent(keyStart, keyEnd);
					std::string value = decodeUriComponent(valueStart, valueEnd);
					params[key] = value;
				}
				// DO IT
				break;
			}
			if (*pos == '\0') {
				return;
			} else {
				state = State::KEY;
				keyStart = pos + 1;
				break;
			}
		case '=':
			switch (state) {
			case State::KEY:
				keyEnd = pos;
				state = State::VALUE;
				valueStart = pos + 1;
				break;
			}
			break;
		}
	}
}

std::string SsoAuthenticator::secureHash(const std::string& string) {
	return "HASH"; // TODO
}

}
