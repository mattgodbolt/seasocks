#include "seasocks/ssoauthenticator.h"

#include "seasocks/AccessControl.h"

#include "md5/md5.h"

#include <string.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <stdexcept>

namespace {

int hexDigit(char digit) {
	char c = toupper(digit);
	if (c >= '0' && c < '9') return c - '0';
	return c - 'A' + 10;
}

const char hex[] = "0123456789ABCDEF";

template<int length>
std::string hexStringOf(uint8_t (&data)[length]) {
	std::string s;
	s.resize(length * 2);
	for (int i = 0; i < length; ++i) {
		s[i * 2] = hex[(data[i]>>4) & 0xf];
		s[i * 2 + 1] = hex[data[i] & 0xf];
	}
	return s;
}

class DefaultAccessControl : public SeaSocks::AccessControl {
public:
	virtual bool requiresAuthentication(const char* requestUri) {
		return true;
	}

	bool hasAccess(boost::shared_ptr<SeaSocks::Credentials> credentials, const char* requestUri) {
		return true;
	}
};

}

namespace SeaSocks {

static const int SSO_PROTOCOL_VERSION = 1;

SsoAuthenticator::SsoAuthenticator(const SsoOptions& options) : _options(options) {
	if (!_options.accessController) {
		_options.accessController.reset(new DefaultAccessControl);
	}
}

bool SsoAuthenticator::isBounceBackFromSsoServer(const char* requestUri) const {
	bool prefixMatch = _options.returnPath.compare(0, _options.returnPath.length(), requestUri,  _options.returnPath.length()) == 0;
	if (prefixMatch) {
		char nextChar = requestUri[_options.returnPath.length()];
		return nextChar == '\0' || nextChar == '?';
	} else {
		return false;
	}
}

bool SsoAuthenticator::enabledForPath(const char* requestUri) const {
	return _options.accessController->requiresAuthentication(requestUri);
}

bool SsoAuthenticator::hasAccess(boost::shared_ptr<Credentials> credentials, const char* requestUri) const {
	return _options.accessController->hasAccess(credentials, requestUri);
}

bool SsoAuthenticator::validateSignature(const char* requestUri) const {
	// TODO: Implement this.
	// get 'user' and 'signature' params
	// token = user + '|' + determineBaseUrl
	// return validate(token, signature, _options.authServerCert);
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
		 << "Connection: close\r\n"
		 << "\r\n";
	return true;
}

bool SsoAuthenticator::respondWithRedirectToAuthenticationServer(const char* requestUri, const std::string& requestHost, std::ostream& response, std::string& error) {
	std::stringstream baseUrl;
	if (_options.basePath.empty()) {
		baseUrl	<< "http://" << requestHost;
	} else {
		baseUrl << _options.basePath;
	}
	std::stringstream targetUrl;
	targetUrl << baseUrl.str() << _options.returnPath << "?continue=" << encodeUriComponent(requestUri);
	std::stringstream redirectUrl;
	redirectUrl << _options.authServer << "/login"
		    << "?basePath=" << encodeUriComponent(baseUrl.str())
		    << "&target=" << encodeUriComponent(targetUrl.str())
		    << "&version=" << SSO_PROTOCOL_VERSION;
	if (!_options.theme.empty()) {
		redirectUrl << "&theme=" << encodeUriComponent(_options.theme);
	}
	response << "HTTP/1.1 307 Temporary Redirect\r\n"
		 << "Location: " << redirectUrl.str() << "\r\n"
		 << "Connection: close\r\n"
		 << "\r\n";
	return true;
}

void SsoAuthenticator::extractCredentialsFromLocalCookie(const std::string& cookieString, boost::shared_ptr<Credentials> target) const {
	target->authenticated = false;
	target->username = "";

	std::map<std::string,std::string> cookieValues;
	parseCookie(cookieString, cookieValues);
	if (cookieValues.count(_options.authCookieName)) {
		std::string authCookie = cookieValues[_options.authCookieName];
		size_t delim = authCookie.find('|');
		if (delim != std::string::npos) {
			std::string username = authCookie.substr(0, delim);
			std::string hash = authCookie.substr(delim + 1);
			if (secureHash(username) == hash) {
				target->authenticated = true;
				target->username = username;
			}
		}	
	}
}

bool SsoAuthenticator::requestExplicityForbidsDrwSsoRedirect() const {
	// TODO: Implement this
	// return header['drw-sso-no-redirect'] == "true"
	return false;
}

std::string SsoAuthenticator::encodeUriComponent(const std::string& value) {
	std::vector<char> result;
	result.reserve(value.size() * 2);
	for (int i = 0, l = value.length(); i < l; ++i) {
		char c = value[i];
		if (c == ' ') {
			result.push_back('+');
		} else if (isalnum(c)) {
			result.push_back(c);
		} else {
			result.push_back('%');
			result.push_back(hex[(c>>4) & 0xf]);
			result.push_back(hex[c & 0xf]);
		}
	}
	return std::string(result.data(), result.size());
}

std::string SsoAuthenticator::decodeUriComponent(const char* value, const char* end) {
	std::string result;
	char hexbuffer[3];
	hexbuffer[2] = '\0';
	while (value != end) {
		if (*value == '%' && value + 2 < end && isxdigit(*(value + 1)) && isxdigit(*(value + 2))) {
			result += (char) (hexDigit(value[1]) << 4 | hexDigit(value[2]));
			value += 3;
		} else if (*value == '+') {
			value++;
			result += ' ';
		} else {
			result += *(value++);
		}
	}
	return result;
}

void SsoAuthenticator::parseCookie(const std::string& cookieString, std::map<std::string, std::string>& cookieValues) {
	enum State {
		BEFORE_KEY, KEY, EQUALS, VALUE, QUOTED_VALUE
	};
	State state = State::BEFORE_KEY;
	const char* keyStart = NULL;
	const char* keyEnd = NULL;
	const char* valueStart = NULL;
	const char* valueEnd = NULL;
	std::stringstream quotedBuffer;
	const char* cookieCStr = cookieString.c_str();
	for (const char *pos = cookieCStr;; ++pos) {
		switch (state) {
		case State::BEFORE_KEY:
			switch (*pos) {
			case '\0':
				return;
			case ' ':
			case ';':
				break;
			default:
				state = State::KEY;
				keyStart = pos;
				break;
			}
			break;
		case State::KEY:
			switch (*pos) {
			case '\0':
				return;
			case '=':
				keyEnd = pos;
				state = State::EQUALS;
				break;
			}
			break;
		case State::EQUALS:
			switch (*pos) {
			case '\0':
				return;
			case '"':
				if (*(pos + 1) == '\0') {
					return;
				} else {
					quotedBuffer.str("");
					state = State::QUOTED_VALUE;
				}
				break;
			default:
				valueStart = pos;
				state = State::VALUE;
                break;
			}
			break;
		case State::VALUE:
			switch (*pos) {
			case '\0':
			case ';':
				valueEnd = pos;
				cookieValues[std::string(keyStart, keyEnd - keyStart)] = std::string(valueStart, valueEnd - valueStart);
				if (*pos == '\0') {
					return;
				} else {
					keyStart = pos;
					state = State::BEFORE_KEY;
					break;
				}
			}
			break;
		case State::QUOTED_VALUE: 
			switch (*pos) {
			case '\\':
				pos++;
				if (*pos == '\0') {
					return;
				} else {
					quotedBuffer << *pos;
				}
				break;
			case '\0':
			case '"':
				valueEnd = pos;
				cookieValues[std::string(keyStart, keyEnd - keyStart)] = quotedBuffer.str();
				if (*pos == '\0') {
					return;
				} else {
					state = State::BEFORE_KEY;
					break;
				}
			default:
				quotedBuffer << *pos;
                break;
			}
			break;
		}
	}
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

std::string SsoAuthenticator::secureHash(const std::string& string) const {
	md5_state_t state;
	md5_init(&state);
	md5_byte_t pipe = '|';
	md5_append(&state, reinterpret_cast<const md5_byte_t*>(_options.localAuthKey.c_str()), _options.localAuthKey.size());
	md5_append(&state, &pipe, 1);
	md5_append(&state, reinterpret_cast<const md5_byte_t*>(string.c_str()), string.size());
	md5_append(&state, &pipe, 1);
	md5_append(&state, reinterpret_cast<const md5_byte_t*>(_options.localAuthKey.c_str()), _options.localAuthKey.size());
	uint8_t digest[16];
	md5_finish(&state, digest);
	return hexStringOf(digest);
}

std::string SsoOptions::createRandomLocalKey() {
	std::ifstream ifs("/dev/urandom", std::ios::binary);
	if (!ifs) {
		throw std::runtime_error("Unable to get random seed");
	}
	uint8_t buf[32];
	if (ifs.read(reinterpret_cast<char*>(&buf[0]), sizeof(buf)).gcount() != sizeof(buf)) {
		throw std::runtime_error("Unable to read from random source");
	}
	return hexStringOf(buf);
}

}
