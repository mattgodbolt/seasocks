#include "seasocks/StringUtil.h"
#include "seasocks/util/CrackedUri.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#define THROW(stuff) \
    do {\
        std::ostringstream err; \
        err << stuff; \
        throw std::runtime_error(err.str()); \
    } while (0);

namespace {

char fromHex(char c) {
    c = tolower(c);
    return c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
}

std::string unescape(std::string uri) {
    SeaSocks::replace(uri, "+", " ");
    size_t pos = 0;
    while (pos < uri.size()) {
        pos = uri.find('%', pos);
        if (pos == uri.npos) break;
        if (pos + 2 > uri.size()) {
            THROW("Truncated uri: '" << uri << "'");
        }
        if (!isxdigit(uri[pos + 1]) || !isxdigit(uri[pos + 2])) {
            THROW("Bad digit in uri: '" << uri << "'");
        }
        auto hex = (fromHex(uri[pos + 1]) << 4) | fromHex(uri[pos + 2]);
        uri = uri.substr(0, pos) + std::string(1, hex) + uri.substr(pos + 3);
        ++pos;
    }
    return uri;
}

}

namespace SeaSocks {

CrackedUri::CrackedUri(const std::string& uri) {
    if (uri.empty() || uri[0] != '/') {
        THROW("Malformed URI: '" << uri << "'");
    }
    auto endOfPath = uri.find('?');
    std::string path;
    std::string remainder;
    if (endOfPath == uri.npos) {
        path = uri.substr(1);
    } else {
        path = uri.substr(1, endOfPath - 1);
        remainder = uri.substr(endOfPath + 1);
    }

    _path = SeaSocks::split(path, '/');
    std::transform(_path.begin(), _path.end(), _path.begin(), unescape);

    auto splitRemainder = SeaSocks::split(remainder, '&');
    for (auto iter = splitRemainder.cbegin(); iter != splitRemainder.cend(); ++iter) {
        if (iter->empty()) continue;
        auto split = SeaSocks::split(*iter, '=');
        std::transform(split.begin(), split.end(), split.begin(), unescape);
        if (split.size() == 1) {
            _queryParams.insert(std::make_pair(split[0], std::string()));
        } else if (split.size() == 2) {
            _queryParams.insert(std::make_pair(split[0], split[1]));
        } else {
            THROW("Malformed URI, two many = in query: '" << uri << "'");
        }
    }
}

bool CrackedUri::hasParam(const std::string& param) const {
    return _queryParams.find(param) != _queryParams.end();
}

std::string CrackedUri::queryParam(const std::string& param, const std::string& def) const {
    auto found = _queryParams.find(param);
    return found == _queryParams.end() ? def : found->second;
}

std::vector<std::string> CrackedUri::allQueryParams(const std::string& param) const {
    std::vector<std::string> params;
    for (auto iter = _queryParams.find(param); iter != _queryParams.end() && iter->first == param; ++iter)
        params.push_back(iter->second);
    return params;
}

}
