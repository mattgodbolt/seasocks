#include "tinytest.h"

#include <string>
#include "seasocks/ssoauthenticator.h"
#include <iostream>
#include <sstream>

using namespace SeaSocks;

void checks_uri_prefix_to_see_if_bounceback() {
	SsoOptions options = SsoOptions::test();
	options.returnPath = "/__bounceback";
	SsoAuthenticator sso(options);
	
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer("/__bounceback"));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer("/__bounceback?"));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer("/__bounceback?foo=blah&gbgg=423"));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer("/__bounceback?"));
	
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/another"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/another/__bounceback"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/not__bounceback"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/__bouncebackNOT"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/another?foo=blah&gbgg=423"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/__bounceb"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/_bounceback"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/__bouncebac"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/"));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer("/?"));
}

void parses_uri_parameters() {
	using namespace std;
	map<string, string> params;

	SsoAuthenticator::parseUriParameters("url?name=joe&age=99&foo=HELLO%2F%26%3D%20WORLD%20%2B%25++%3B", params);
	ASSERT_EQUALS("joe", params["name"]);
	ASSERT_EQUALS("99", params["age"]);
	ASSERT_EQUALS("HELLO/&= WORLD +%  ;", params["foo"]);
}

void skips_bad_uri_encodings() {
	using namespace std;
	map<string, string> params;

	SsoAuthenticator::parseUriParameters("url?bad1=abc%2x%&bad2=%5", params);
	ASSERT_EQUALS("abc%2x%", params["bad1"]);
	ASSERT_EQUALS("%5", params["bad2"]);
}

void encodes_uri_components() {
	ASSERT_EQUALS("hello", SsoAuthenticator::encodeUriComponent("hello"));
	ASSERT_EQUALS("HELLO%2F%26%3D+WORLD+%2B%25++%3B",
                      SsoAuthenticator::encodeUriComponent("HELLO/&= WORLD +%  ;"));
}

void extract_credentials_from_local_cookie() {
	SsoOptions options = SsoOptions::test();
	SsoAuthenticator sso(options);
	boost::shared_ptr<Credentials> credentials(new Credentials());

	// happy path
	sso.extractCredentialsFromLocalCookie("_auth=joe|HASH", credentials);
	ASSERT_EQUALS(true, credentials->authenticated);
	ASSERT_EQUALS("joe", credentials->username);

	// no cookie 
	credentials.reset(new Credentials());
	sso.extractCredentialsFromLocalCookie("", credentials);
	ASSERT_EQUALS(false, credentials->authenticated);
	ASSERT_EQUALS("", credentials->username);

	// invalid hash
	credentials.reset(new Credentials());
	sso.extractCredentialsFromLocalCookie("_auth=joe|BADHASH", credentials);
	ASSERT_EQUALS(false, credentials->authenticated);
	ASSERT_EQUALS("", credentials->username);

	// other cookies (ignored)
	credentials.reset(new Credentials());
	sso.extractCredentialsFromLocalCookie("foo=bar;x=\" _auth=ignoreme \"_auth=joe|HASH;blah=x", credentials);
	ASSERT_EQUALS(true, credentials->authenticated);
	ASSERT_EQUALS("joe", credentials->username);
}

void parses_cookies() {
	using namespace std;
	map<string, string> cookies;

	// single value
	SsoAuthenticator::parseCookie("hello=world", cookies);
	ASSERT_EQUALS("world", cookies["hello"]);

	// multiple values
	cookies.clear();
	SsoAuthenticator::parseCookie("hello=world;bye=you;blah=foo", cookies);
	ASSERT_EQUALS("world", cookies["hello"]);
	ASSERT_EQUALS("you", cookies["bye"]);
	ASSERT_EQUALS("foo", cookies["blah"]);

	// trailing ';'
	cookies.clear();
	SsoAuthenticator::parseCookie("hello=world;", cookies);
	ASSERT_EQUALS("world", cookies["hello"]);

	// quoted values
	cookies.clear();
	SsoAuthenticator::parseCookie("hello=\"world ;\\\"%= \";bye=\"you\"", cookies);
	ASSERT_EQUALS("world ;\"%= ", cookies["hello"]);
	ASSERT_EQUALS("you", cookies["bye"]);
}

void parses_bounceback_params_and_generates_redirect() {
	SsoOptions options = SsoOptions::test();
	options.returnPath = "/__bounceback";
	SsoAuthenticator sso(options);

	std::stringstream response;
	std::string error;
	sso.respondWithLocalCookieAndRedirectToOriginalPage("/__bounceback?user=joe&continue=%2fpage", response, error);
	std::cout << response.str();
}

int main(int argc, const char* argv[]) {
	RUN(checks_uri_prefix_to_see_if_bounceback);
	RUN(parses_uri_parameters);
	RUN(skips_bad_uri_encodings);
	RUN(encodes_uri_components);
	RUN(extract_credentials_from_local_cookie);
	RUN(parses_cookies);
	RUN(parses_bounceback_params_and_generates_redirect);
	return TEST_REPORT();
}
 
