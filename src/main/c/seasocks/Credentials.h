#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>

namespace seasocks {

struct Credentials {
    /**
     * Whether user was successfuly authenticated.
     */
    bool authenticated;

    /**
     * e.g. "mgodbolt" (or "" for unauthenticated users)
     */
    std::string username;

    /**
     * Groups the user is in.
     */
    std::set<std::string> groups;

    /**
     * Attributes for the user.
     */
    std::map<std::string, std::string> attributes;

    Credentials(): authenticated(false) {}
};

inline std::ostream &operator<<(std::ostream &os, const Credentials& credentials) {
    os << "{authenticated:" << credentials.authenticated << ", username:'" << credentials.username << "', groups: {";
    for (auto it = credentials.groups.begin(); it != credentials.groups.end(); ++it) {
        if (it != credentials.groups.begin()) os << ", ";
        os << *it;
    }
    os << "}, attrs: {";
    for (auto it = credentials.attributes.begin(); it != credentials.attributes.end(); ++it) {
        if (it != credentials.attributes.begin()) os << ", ";
        os << it->first << "=" << it->second;
    }
    return os << "}}";
}

}  // namespace seasocks
