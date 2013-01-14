#pragma once

#include "seasocks/Request.h"

#include <map>
#include <vector>

namespace seasocks {

class PageRequest : public seasocks::Request {
	std::shared_ptr<seasocks::Credentials> _credentials;
	const sockaddr_in _remoteAddress;
	const std::string _requestUri;
	const Verb _verb;
	const size_t _contentLength;
	std::vector<uint8_t> _content;
	std::map<std::string, std::string> _headers;

public:
	PageRequest(
			const sockaddr_in& remoteAddress,
			const std::string& requestUri,
			const char* verb,
			size_t contentLength,
			const std::map<std::string, std::string>& headers);

	virtual Verb verb() const {
		return _verb;
	}

	virtual std::shared_ptr<seasocks::Credentials> credentials() const {
		return _credentials;
	}

	virtual const sockaddr_in& getRemoteAddress() const {
		return _remoteAddress;
	}

	virtual const std::string& getRequestUri() const {
		return _requestUri;
	}

	virtual size_t contentLength() const {
		return _contentLength;
	}

	virtual const uint8_t* content() const {
		return _contentLength > 0 ? &_content[0] : NULL;
	}

	virtual bool hasHeader(const std::string& name) const {
		return _headers.find(name) != _headers.end();
	}

	virtual std::string getHeader(const std::string& name) const {
		auto iter = _headers.find(name);
		return iter == _headers.end() ? std::string() : iter->second;
	}

	bool consumeContent(std::vector<uint8_t>& buffer);
};

}  // namespace seasocks
