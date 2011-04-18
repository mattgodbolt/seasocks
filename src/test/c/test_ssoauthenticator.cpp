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
	RUN(parses_bounceback_params_and_generates_redirect);
	return TEST_REPORT();
}
 
