#ifndef _SEASOCKS_CREDENTIALS_H_
#define _SEASOCKS_CREDENTIALS_H_

#include <string>

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

// TODO: override << operator

}  // namespace SeaSocks

#endif  // _SEASOCKS_CREDENTIALS_H_
