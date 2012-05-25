#ifndef _SEASOCKS_SSO_AUTHENTICATOR_H_
#define _SEASOCKS_SSO_AUTHENTICATOR_H_

#include "seasocks/credentials.h"
#include "seasocks/AccessControl.h"
#include "seasocks/Request.h"
#include "seasocks/Response.h"

#include <memory>

#include <string>
#include <map>
#include <set>
#include <strings.h>

namespace SeaSocks {


/**
 * Options for SSO client.
 *
 * Use the static methods to construct populated options for environments:
 *
 *   SsoOptions::production() : The default SSO server: https://signon.drwholdings.com. This
 *                              is the only one you should use in production.
 *
 *   SsoOptions::test()       : Use https://signon-test.drwholdings.com, which behaves like the 
 *                              production instance, except it allows you to logon as any user.
 *                              This is useful during development/testing for simulating other
 *                              users. Don't use for a production system!
 *
 *   SsoOptions::local()      : Use http://localhost:5003 as the SSO server.
 *                              This is useful if you're making changes to the SSO server and
 *                              want to test locally.
 *
 * This can also be selected using a string: SsoOptions::environment(string) - this is useful
 * for apps that configure the environment using a config file.
 *
 * In addition, you may override specific fields for more control. Commonly overridden fields
 * are 'basePath' and 'theme' - see comments below.
 */
struct SsoOptions {

	/**
	 * The URL of the SSO authentication server. e.g.'https://signon.drwholdings.com'. 
	 */
	std::string authServer;

	/**
	 * The ASCII public key/certificate of the SSO server to validate it is who it says it is.
	 */
	std::string authServerCertificate;

	/**
	 * The magic URL the SSO server will send credentials back to this server on. This should be
	 * a path that's not used by anything else in your application as SSO will intercept it. 
	 * Defaults to "/__auth"
	 */
	std::string returnPath;

	/**
	 * Base path of this web-application. The default value "", which means the web-app will try to
	 * figure it out itself. In some cases, this may be wrong, e.g. if using a DNS alias or other 
	 * web-server frontend. Format should look like 'http://someserver' or 'http://someserver/context'.
	 * NOTE: It should NEVER contain a trailing slash!
	 */
	std::string basePath;

	/**
	 * Visual theme SSO auth server should use for login screen.
	 * To see a list of available themes, visit https://signon.drwholdings.com/themes in a browser 
	 * Default is "", which will use the SSO server's default DRW theme. 
	 * For dark background sites, try "carbon".
	 */
	std::string theme;

	/**
	 * Name of cookie to use to store local username. Defaults to "_auth".
	 */
	std::string authCookieName;

	/**
	 * A secret known only to this app, used to sign and verify the local cookie. 
	 *    SsoOptions

	 *
	 * The default value is a random string generated at startup. Application restarts
	 * will invalidate the cookie, causing the user to be re-authenticated by the SSO
	 * server, though this is transparent to the user.
	 */
	std::string localAuthKey;

	/**
	 * Smart pointer to an object that will perform access control.
	 *
	 * The default value is null, which creates an access control object that requires
	 * login for all domains, but doesn't restrict by username.
	 */
	std::shared_ptr<AccessControl> accessController;

	enum ProtocolVersion { VERSION_1 = 1, VERSION_2 = 2};

	/**
	 * Version of the SSO protocol to use in requests.
	 */
	ProtocolVersion protocolVersion;

    /**
     * SSO Protocol 2 and above only: Set of LDAP groups we would like information on.
     */
    std::set<std::string> requestGroups;

    /**
     * SSO Protocol 2 and above only: Set of user attributes we would like information on.
     */
    std::set<std::string> requestUserAttributes;

    SsoOptions() : protocolVersion(VERSION_2) {}

	/**
	 * Create options that will 'just work' for default SSO server: https://signon.drwholdings.com. 
	 * This is the only one you should use in production (though you may also use it in dev/testing
	 * too if you want to authenticate real users).
	 */
	static SsoOptions production() {
		SsoOptions options;
		options.authServer = "https://signon.drwholdings.com";
		options.authServerCertificate = 
			"-----BEGIN CERTIFICATE-----\n"
			"MIICZTCCAc4CCQCYD7H7PN9LPTANBgkqhkiG9w0BAQUFADB3MQswCQYDVQQGEwJV\n"
			"UzELMAkGA1UECBMCSUwxEDAOBgNVBAcTB0NoaWNhZ28xDDAKBgNVBAoTA0RSVzET\n"
			"MBEGA1UEAxMKSm9lIFdhbG5lczEmMCQGCSqGSIb3DQEJARYXandhbG5lc0Bkcndo\n"
			"b2xkaW5ncy5jb20wHhcNMTAwOTE0MTgzNTM2WhcNMTAxMDE0MTgzNTM2WjB3MQsw\n"
			"CQYDVQQGEwJVUzELMAkGA1UECBMCSUwxEDAOBgNVBAcTB0NoaWNhZ28xDDAKBgNV\n"
			"BAoTA0RSVzETMBEGA1UEAxMKSm9lIFdhbG5lczEmMCQGCSqGSIb3DQEJARYXandh\n"
			"bG5lc0Bkcndob2xkaW5ncy5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGB\n"
			"AJ4l24+U5W8wBkcgLF7MvrpXmmccjpPbzwfT/TDaR5Qqxs7ckPrqAAppB/QYTtvA\n"
			"yVzI8NPvv/t/Eq94Vi+OLzRAK+u82KAcO6RC6OTSwD6+K6j1ImGp4nZuajPQtXnF\n"
			"3wMi9RSU5i+nRf94r/i6uDicE4JYnAoqaxnMzyzKY2KRAgMBAAEwDQYJKoZIhvcN\n"
			"AQEFBQADgYEAUSL2mX7VSS67RGaQn9O1+0CqBdFyHJic3lUdELIRVk6kx/eCVcTq\n"
			"SePE7CqeZWFq5yhRzXupFyUMlQxSTpk63BXlGffEYKLcr5BCCpv/nY6241qqQT48\n"
			"J+nbXDE0+3reJQ/R9f/jwC6siM1E5Yilh/c6gJZKyLi6zaJ/m7E/7y4=\n"
			"-----END CERTIFICATE-----\n";
		options.returnPath = "/__auth";
		options.basePath = "";
		options.theme = "";
		options.authCookieName = "_auth";
		options.localAuthKey = createRandomLocalKey();
		return options;
	}

	/**
	 * Use https://signon-test.drwholdings.com, which behaves like the 
	 * production instance, except it allows you to logon as any user.
	 * This is useful during development/testing for simulating other
	 * users. Don't use for a production system!
	 */
	static SsoOptions test() {
		SsoOptions options = production();
		options.authServer = "https://signon-test.drwholdings.com";
		options.authServerCertificate =
			"-----BEGIN CERTIFICATE-----\n"
			"MIICZTCCAc4CCQDr9qhVbBy7azANBgkqhkiG9w0BAQUFADB3MQswCQYDVQQGEwJV\n"
			"UzELMAkGA1UECBMCSUwxEDAOBgNVBAcTB0NoaWNhZ28xDDAKBgNVBAoTA0RSVzET\n"
			"MBEGA1UEAxMKSm9lIFdhbG5lczEmMCQGCSqGSIb3DQEJARYXandhbG5lc0Bkcndo\n"
			"b2xkaW5ncy5jb20wHhcNMTAwOTE0MTgzNjIxWhcNMTAxMDE0MTgzNjIxWjB3MQsw\n"
			"CQYDVQQGEwJVUzELMAkGA1UECBMCSUwxEDAOBgNVBAcTB0NoaWNhZ28xDDAKBgNV\n"
			"BAoTA0RSVzETMBEGA1UEAxMKSm9lIFdhbG5lczEmMCQGCSqGSIb3DQEJARYXandh\n"
			"bG5lc0Bkcndob2xkaW5ncy5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGB\n"
			"AOxMAYtRJF6u8QgGc6vK0WHKzQb8tZVG9f5aYCvW3FxgvHbXA3+1lGGUcNapVe6p\n"
			"yUVwP9Jno0mqQ0ETfW+K8uyArmZgaf2AmgnUMJxcdl8XO/SjSzL7/YCpekrlraNX\n"
			"7SeWXc6ToN9QVdLxnaDsm4BFO3DA5c0TQw5TqRzs6fwfAgMBAAEwDQYJKoZIhvcN\n"
			"AQEFBQADgYEAGZpu6DHXzhP+YY0o4HAH1yh0dlo9c1TBbyBlBykMfUjJxd4YoAs2\n"
			"YCDyN6oLUcmfpDs5dXygZhmL8IBuyX3LZoxied1KinPczJln/Zigm512eIBSdcjO\n"
			"+JweJdFf5oJpxcXfSj3svjsVzTGr6AIRchCHLAhpyUJDRkJrjEnNEdM=\n"
			"-----END CERTIFICATE-----\n";
		return options;
	}

	/**
	 * Use http://localhost:5003 as the SSO server.
	 * This is useful if you're making changes to the SSO server and ant to test locally.
	 */
	static SsoOptions local() {
		SsoOptions options = test();
		options.authServer = "http://localhost:5003";
		return options;
	}

  /**
   * Returns SsoOptions for "production", "test", or "local" environment
   * based on string. Useful for apps that configure SSO from a config file.
   */
  static SsoOptions environment(const std::string& name) {
    if (::strcasecmp(name.c_str(), "prod") == 0 || 
        ::strcasecmp(name.c_str(), "production") == 0) {
      return production();
    } else if (::strcasecmp(name.c_str(), "test") == 0) {
      return test();
    } else if (::strcasecmp(name.c_str(), "dev") == 0 ||
               ::strcasecmp(name.c_str(), "development") == 0 ||
               ::strcasecmp(name.c_str(), "local") == 0) {
      return local();
    } else {
      return SsoOptions();
    }
  }

  static std::string createRandomLocalKey();
};

class SsoAuthenticator {
public:
	SsoAuthenticator(const SsoOptions& options);
	bool enabledFor(const Request& request) const;
	bool hasAccess(const Request& request) const;
	bool isBounceBackFromSsoServer(const Request& request) const;
	bool validateSignature(const Request& request) const;
	std::shared_ptr<Response> respondWithLocalCookieAndRedirectToOriginalPage(const Request& request);
	std::shared_ptr<Response> respondWithRedirectToAuthenticationServer(const Request& request);
	void extractCredentialsFromLocalCookie(Request& request) const;
	bool requestExplicityForbidsDrwSsoRedirect(const Request& request) const;
	std::string secureHash(const std::string& string) const;
	static void parseUriParameters(const std::string& uri, std::map<std::string, std::string>& params);
    static std::string decodeUriComponent(const char* value, const char* end);
    static std::string decodeUriComponent(const std::string& value);
    static std::string encodeUriComponent(const std::string& value, bool ssoWorkaround = false);
	static void parseCookie(const std::string& cookieValue, std::map<std::string, std::string>& params);
private:
	SsoOptions _options;
};

}  // namespace SeaSocks

#endif  // _SEASOCKS_SSO_AUTHENTICATOR_H_
