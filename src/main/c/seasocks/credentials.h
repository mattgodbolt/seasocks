#ifndef _SEASOCKS_CREDENTIALS_H_
#define _SEASOCKS_CREDENTIALS_H_

#include <string>
#include <iostream>

namespace SeaSocks {

struct Credentials {
	
	/**
	 * Whether user was successfuly authenticated.
	 */
	bool authenticated;
	
	/**
	 * e.g. "mgodbolt" (or "" for unauthenticated users)
	 */
	std::string username;
	
	// TODO: Support SSO V2 for additional fields:
	// std::set<std::string> groups;
	// std::map<std::string, std::string> attributes;
	
	Credentials(): authenticated(false), username("") {
	}
	
};

inline std::ostream &operator<<(std::ostream &os, Credentials& credentials) {
	return os << "{authenticated:" << credentials.authenticated << ", username:'" << credentials.username << "'}";
}

}  // namespace SeaSocks

#endif  // _SEASOCKS_CREDENTIALS_H_
