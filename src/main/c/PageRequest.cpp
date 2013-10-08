#include "internal/PageRequest.h"

#include <cstring>

namespace seasocks {

PageRequest::PageRequest(
        const sockaddr_in& remoteAddress,
        const std::string& requestUri,
        const Verb verb,
        size_t contentLength,
        std::unordered_map<std::string, std::string>&& headers) :
            _credentials(std::shared_ptr<Credentials>(new Credentials())),
            _remoteAddress(remoteAddress),
            _requestUri(requestUri),
            _verb(verb),
            _contentLength(contentLength),
            _headers(std::move(headers)) {
}

bool PageRequest::consumeContent(std::vector<uint8_t>& buffer) {
    if (buffer.size() < _contentLength) return false;
    if (buffer.size() == _contentLength) {
        _content.swap(buffer);
    } else {
        _content.assign(buffer.begin(), buffer.begin() + _contentLength);
        buffer.erase(buffer.begin(), buffer.begin() + _contentLength);
    }
    return true;
}

}  // namespace seasocks
