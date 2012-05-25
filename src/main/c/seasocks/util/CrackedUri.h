#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace SeaSocks {

class CrackedUri {
    std::vector<std::string> _path;
    std::unordered_multimap<std::string, std::string> _queryParams;
public:
    CrackedUri(const std::string& uri);

    const std::vector<std::string>& path() const { return _path; }
    const std::unordered_multimap<std::string, std::string> queryParams() const { return _queryParams; }

    bool hasParam(const std::string& param) const;

    // Returns the user-supplied default if not present. Returns the first found in the case of multiple params.
    std::string queryParam(const std::string& param, const std::string& def = std::string()) const;

    std::vector<std::string> allQueryParams(const std::string& param) const;
};

}
