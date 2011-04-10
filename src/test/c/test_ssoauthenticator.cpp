#include "tinytest.h"

#include <string>
#include "seasocks/ssoauthenticator.h"

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

int main(int argc, const char* argv[]) {
	RUN(checks_uri_prefix_to_see_if_bounceback);
	return TEST_REPORT();
}
 
