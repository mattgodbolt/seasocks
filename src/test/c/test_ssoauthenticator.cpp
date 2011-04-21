#include "tinytest.h"

#include <string>
#include "seasocks/ssoauthenticator.h"
#include <iostream>
#include <sstream>
#include <string.h>

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
	ASSERT_EQUALS("%01%02%03%FF", SsoAuthenticator::encodeUriComponent("\x01\x02\x03\xff"));
	ASSERT_EQUALS("%01%02%03%FFhElLoMuM", SsoAuthenticator::encodeUriComponent("\x01\x02\x03\xff" "hElLoMuM"));
}

void decodes_uri_components() {
	static const char hello[] = "hello";
	ASSERT_EQUALS("hello", SsoAuthenticator::decodeUriComponent(hello, hello + strlen(hello)));
	static const char hello_world[] = "hello%2c+world";
	ASSERT_EQUALS("hello, world", SsoAuthenticator::decodeUriComponent(hello_world, hello_world + strlen(hello_world)));
	static const char truncated[] = "%1";
	ASSERT_EQUALS("%1", SsoAuthenticator::decodeUriComponent(truncated, truncated + strlen(truncated)));
	static const char helloMum[] = "%01%02%03%FFhElLoMuM";
	ASSERT_EQUALS("\x01\x02\x03\xff" "hElLoMuM", SsoAuthenticator::decodeUriComponent(helloMum, helloMum + strlen(helloMum)));
}

void extract_credentials_from_local_cookie() {
	SsoOptions options = SsoOptions::test();
	options.localAuthKey = "I AM NOT SECURE";
	SsoAuthenticator sso(options);
	boost::shared_ptr<Credentials> credentials(new Credentials());

	// happy path
	sso.extractCredentialsFromLocalCookie("_auth=joe|49A85A78D89CE4D36997C5B48940A2C6", credentials);
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
	sso.extractCredentialsFromLocalCookie("foo=bar;x=\" _auth=ignoreme \"_auth=joe|49A85A78D89CE4D36997C5B48940A2C6;blah=x", credentials);
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

void redirects_to_sso_server() {
	SsoOptions options = SsoOptions::test();
	options.returnPath = "/__bounceback";
	options.authServer = "https://the-auth-server:10000";
	SsoAuthenticator sso(options);

	std::stringstream response;
	std::string error;
	sso.respondWithRedirectToAuthenticationServer("/mypage?foo", "myserver:8080", response, error);

	std::string expectedResponse = 
		"HTTP/1.1 307 Temporary Redirect\r\n"
		"Location: https://the-auth-server:10000/login?basePath=http%3A%2F%2Fmyserver%3A8080&target=http%3A%2F%2Fmyserver%3A8080%2F%5F%5Fbounceback%3Fcontinue%3D%252Fmypage%253Ffoo&version=1\r\n"
		"\r\n";
	ASSERT_EQUALS(expectedResponse, response.str());
}

void parses_bounceback_params_and_generates_redirect() {
	SsoOptions options = SsoOptions::test();
	options.localAuthKey = "I AM NOT SECURE";
	options.returnPath = "/__bounceback";
	SsoAuthenticator sso(options);

	std::stringstream response;
	std::string error;
	sso.respondWithLocalCookieAndRedirectToOriginalPage("/__bounceback?user=joe&continue=%2fpage", response, error);

	std::string expectedResponse = 
		"HTTP/1.1 307 Temporary Redirect\r\n"
		"Location: /page\r\n"
		"Set-Cookie: _auth=joe|49A85A78D89CE4D36997C5B48940A2C6\r\n"
		"\r\n";
	ASSERT_EQUALS(expectedResponse, response.str());
}

void returns_same_hash_each_time() {
	SsoOptions options = SsoOptions::test();
	SsoAuthenticator sso(options);
	std::string hash = sso.secureHash("Wallaby");
	ASSERT_EQUALS(hash, sso.secureHash("Wallaby"));
	ASSERT("Not expecting a hash collision", hash != sso.secureHash("A different string"));
}

void uses_a_random_hash() {
	SsoOptions options1 = SsoOptions::production();
	SsoOptions options2 = SsoOptions::production();
	ASSERT("Expecting different hashes", options1.localAuthKey != options2.localAuthKey);
}

int main(int argc, const char* argv[]) {
	RUN(checks_uri_prefix_to_see_if_bounceback);
	RUN(parses_uri_parameters);
	RUN(skips_bad_uri_encodings);
	RUN(encodes_uri_components);
	RUN(decodes_uri_components);
	RUN(extract_credentials_from_local_cookie);
	RUN(parses_cookies);
	RUN(parses_bounceback_params_and_generates_redirect);
	RUN(returns_same_hash_each_time);
	RUN(uses_a_random_hash);
	RUN(redirects_to_sso_server);
	return TEST_REPORT();
}
 
