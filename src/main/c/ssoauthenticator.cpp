#include "seasocks/ssoauthenticator.h"

#include "seasocks/AccessControl.h"
#include "seasocks/ResponseBuilder.h"
#include "seasocks/stringutil.h"

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
	virtual bool requiresAuthentication(const SeaSocks::Request& request) {
		return true;
	}

	bool hasAccess(const SeaSocks::Request& request) {
		return true;
	}
};

void appendSsoSet(std::ostream& os, const char* name, const std::set<std::string>& set) {
    if (set.empty()) return;
    os << "&" << name << "=";
    bool first = true;
    for (auto it = set.cbegin(); it != set.cend(); ++it) {
        if (!first) os << "%26";
        first = false;
        os << SeaSocks::SsoAuthenticator::encodeUriComponent(*it, true);
    }
}

}

namespace SeaSocks {

SsoAuthenticator::SsoAuthenticator(const SsoOptions& options) : _options(options) {
	if (!_options.accessController) {
		_options.accessController.reset(new DefaultAccessControl);
	}
}

bool SsoAuthenticator::isBounceBackFromSsoServer(const Request& request) const {
    auto requestUri = request.getRequestUri();
	bool prefixMatch = _options.returnPath.compare(0, _options.returnPath.length(), requestUri.c_str(), _options.returnPath.length()) == 0;
	if (prefixMatch) {
		char nextChar = requestUri[_options.returnPath.length()];
		return nextChar == '\0' || nextChar == '?';
	} else {
		return false;
	}
}

bool SsoAuthenticator::enabledFor(const Request& request) const {
	return _options.accessController->requiresAuthentication(request);
}

bool SsoAuthenticator::hasAccess(const Request& request) const {
	return _options.accessController->hasAccess(request);
}

bool SsoAuthenticator::validateSignature(const Request& request) const {
	// TODO: Implement this.
	// get 'user' and 'signature' params
	// token = user + '|' + determineBaseUrl
	// return validate(token, signature, _options.authServerCert);
	return true;
}

std::shared_ptr<Response> SsoAuthenticator::respondWithLocalCookieAndRedirectToOriginalPage(const Request& request) {
	std::map<std::string, std::string> params;
	parseUriParameters(request.getRequestUri(), params);
	if (params.count("user") == 0 || params.count("continue") == 0) {
		// Tampering detected.
	    return Response::error(ResponseCode::Forbidden, "Invalid response from SSO server (require user and continue params)");
	}
	std::string user = params["user"];
	std::string continueUrl = params["continue"];
	if (user.empty() || continueUrl.empty()) {
		// Tampering detected.
        return Response::error(ResponseCode::Forbidden, "Invalid response from SSO server (require user and continue params)");
	}
	if (user.find("\r") != std::string::npos
	    || user.find("\n") != std::string::npos
	    || continueUrl.find("://") != std::string::npos
	    || continueUrl.find("\r") != std::string::npos
	    || continueUrl.find("\n") != std::string::npos) {
		// Tampering detected.
        return Response::error(ResponseCode::Forbidden, "Invalid response from SSO server (user or continue params contain invalid chars)");
	}

	std::string cookieValue;
	if (_options.protocolVersion == SsoOptions::VERSION_1) {
	    cookieValue = user;
	} else {
	    cookieValue = user + "|" + encodeUriComponent(params["groups"]) + "|" + encodeUriComponent(params["userData"]);
	}

	return ResponseBuilder(ResponseCode::TemporaryRedirect)
            .withLocation(continueUrl)
            .setsCookie(_options.authCookieName, cookieValue + "|" + encodeUriComponent(secureHash(cookieValue)))
            .closesConnection()
	        .build();
}

std::shared_ptr<Response> SsoAuthenticator::respondWithRedirectToAuthenticationServer(const Request& request) {
	std::stringstream baseUrl;
	if (_options.basePath.empty()) {
		baseUrl	<< "http://" << request.getHeader("Host");
	} else {
		baseUrl << _options.basePath;
	}
	std::stringstream targetUrl;
	targetUrl << baseUrl.str() << _options.returnPath << "?continue=" << encodeUriComponent(request.getRequestUri());
	std::stringstream redirectUrl;
	redirectUrl << _options.authServer << "/login"
		    << "?basePath=" << encodeUriComponent(baseUrl.str())
		    << "&target=" << encodeUriComponent(targetUrl.str())
		    << "&version=" << _options.protocolVersion;
	if (_options.protocolVersion >= SsoOptions::VERSION_2) {
        appendSsoSet(redirectUrl, "groupsRequest", _options.requestGroups);
        appendSsoSet(redirectUrl, "userDataRequest", _options.requestUserAttributes);
	}
	if (!_options.theme.empty()) {
		redirectUrl << "&theme=" << encodeUriComponent(_options.theme);
	}
	return ResponseBuilder(ResponseCode::TemporaryRedirect)
	        .withLocation(redirectUrl.str())
	        .closesConnection()
	        .build();
}

void SsoAuthenticator::extractCredentialsFromLocalCookie(Request& request) const {
    auto cookieString = request.getHeader("Cookie");
    auto target = request.credentials();
	target->authenticated = false;
	target->username = "";

	std::map<std::string,std::string> cookieValues;
	parseCookie(cookieString, cookieValues);
	if (cookieValues.count(_options.authCookieName)) {
		std::string authCookie = cookieValues[_options.authCookieName];
		auto splitUp = split(authCookie, '|');
        if (_options.protocolVersion == SsoOptions::VERSION_1 && splitUp.size() == 2) {
            std::string username = splitUp[0];
            std::string hash = splitUp[1];
            if (secureHash(username) == hash) {
                target->authenticated = true;
                target->username = username;
            }
        } else if (_options.protocolVersion == SsoOptions::VERSION_2 && splitUp.size() == 4) {
            std::string username = splitUp[0];
            std::string groupdata = splitUp[1];
            std::string userdata = splitUp[2];
            std::string hash = splitUp[3];
            auto expectedHash = secureHash(username + "|" + groupdata + "|" + userdata);
            if (expectedHash == hash) {
                target->authenticated = true;
                target->username = username;
                auto groups = split(decodeUriComponent(groupdata), '&');
                for (auto it = groups.begin(); it != groups.end(); ++it) {
                    target->groups.insert(decodeUriComponent(*it));
                }
                auto keyValues = split(decodeUriComponent(userdata), '&');
                for (auto it = keyValues.begin(); it != keyValues.end(); ++it) {
                    size_t equals = it->find("=");
                    if (equals != it->npos) {
                        target->attributes[decodeUriComponent(it->substr(0, equals))] = decodeUriComponent(it->substr(equals + 1));
                    }
                }
            }
        }
	}
}

bool SsoAuthenticator::requestExplicityForbidsDrwSsoRedirect(const Request& request) const {
    return request.getHeader("drw-sso-no-redirect") == "true";
}

std::string SsoAuthenticator::encodeUriComponent(const std::string& value, bool ssoWorkaround) {
	std::vector<char> result;
	result.reserve(value.size() * 2);
	for (int i = 0, l = value.length(); i < l; ++i) {
		char c = value[i];
		if (c == ' ') {
		    if (ssoWorkaround) {
                result.push_back('%');
                result.push_back('2');
                result.push_back('0');
		    } else {
		        result.push_back('+');
		    }
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

std::string SsoAuthenticator::decodeUriComponent(const std::string& value) {
    return decodeUriComponent(&value[0], &value[value.size()]);
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

void SsoAuthenticator::parseUriParameters(const std::string& uri, std::map<std::string, std::string>& params) {
	enum State {
		INITIAL, KEY, VALUE
	};
	State state = State::INITIAL;
	const char* keyStart = NULL;
	const char* keyEnd = NULL;
	const char* valueStart = NULL;
	const char* valueEnd = NULL;
	for (const char *pos = uri.c_str();; ++pos) {
		switch (*pos) {
		case '?':
			switch (state) {
			case State::INITIAL:
				state = State::KEY;
				keyStart = pos + 1;
				break;
			default:
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
            default:
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
            default:
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
