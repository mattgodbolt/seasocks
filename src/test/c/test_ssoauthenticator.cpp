#include "tinytest.h"

#include <string>
#include "seasocks/ssoauthenticator.h"
#include <iostream>
#include <sstream>
#include <string.h>

using namespace SeaSocks;

namespace {

class FakeRequest : public Request {
    std::string _uri;
    std::map<std::string, std::string> _headers;
    boost::shared_ptr<Credentials> _credentials;
public:
    FakeRequest() : _credentials(new Credentials()) {}

    static FakeRequest uri(const std::string& uri) { FakeRequest r; r._uri = uri; return r; }
    static FakeRequest cookie(const std::string& cookie) { FakeRequest r; r._headers["Cookie"] = cookie; return r; }

    FakeRequest& withHeader(const char* header, const char* val) { _headers[header] = val; return *this; }

    virtual Verb verb() const { return Get; }
    virtual boost::shared_ptr<Credentials> credentials() const { return _credentials; }

    virtual const sockaddr_in& getRemoteAddress() const {
        static sockaddr_in sin;
        return sin;
    }

    virtual const std::string& getRequestUri() const {
        return _uri;
    }

    virtual size_t contentLength() const {
        return 0;
    }

    virtual const uint8_t* content() const {
        return NULL;
    }

    virtual bool hasHeader(const std::string& name) const {
        return _headers.find(name) != _headers.end();
    }

    virtual std::string getHeader(const std::string& name) const {
        return _headers.find(name)->second;
    }
};

}

void checks_uri_prefix_to_see_if_bounceback() {
	SsoOptions options = SsoOptions::test();
	options.returnPath = "/__bounceback";
	SsoAuthenticator sso(options);
	
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bounceback")));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bounceback?")));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bounceback?foo=blah&gbgg=423")));
	ASSERT_EQUALS(true, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bounceback?")));
	
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/another")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/another/__bounceback")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/not__bounceback")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bouncebackNOT")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/another?foo=blah&gbgg=423")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bounceb")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/_bounceback")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/__bouncebac")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/")));
	ASSERT_EQUALS(false, sso.isBounceBackFromSsoServer(FakeRequest::uri("/?")));
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

void extract_credentials_from_local_cookie_v1() {
    SsoOptions options = SsoOptions::test();
    options.protocolVersion = SsoOptions::VERSION_1;
    options.localAuthKey = "I AM NOT SECURE";
    SsoAuthenticator sso(options);
    FakeRequest request;

    // happy path
    request = FakeRequest::cookie("_auth=joe|49A85A78D89CE4D36997C5B48940A2C6");
    sso.extractCredentialsFromLocalCookie(request);
    ASSERT_EQUALS(true, request.credentials()->authenticated);
    ASSERT_EQUALS("joe", request.credentials()->username);

    // no cookie
    request = FakeRequest::cookie("");
    sso.extractCredentialsFromLocalCookie(request);
    ASSERT_EQUALS(false, request.credentials()->authenticated);
    ASSERT_EQUALS("", request.credentials()->username);

    // invalid hash
    request = FakeRequest::cookie("_auth=joe|BADHASH");
    sso.extractCredentialsFromLocalCookie(request);
    ASSERT_EQUALS(false, request.credentials()->authenticated);
    ASSERT_EQUALS("", request.credentials()->username);

    // other cookies (ignored)
    request = FakeRequest::cookie("foo=bar;x=\" _auth=ignoreme \"_auth=joe|49A85A78D89CE4D36997C5B48940A2C6;blah=x");
    sso.extractCredentialsFromLocalCookie(request);
    ASSERT_EQUALS(true, request.credentials()->authenticated);
    ASSERT_EQUALS("joe", request.credentials()->username);
}

void extract_credentials_from_local_cookie_v2() {
    SsoOptions options = SsoOptions::test();
    options.protocolVersion = SsoOptions::VERSION_2;
    options.localAuthKey = "I AM NOT SECURE";
    SsoAuthenticator sso(options);
    FakeRequest request;

    // happy path
    request = FakeRequest::cookie("_auth=joe|foo%26bar%26baz|fullName%3DJoe%20Walnes%26baz%3Dbongo|68F544D5E0358692E43BFD2F515C05E8");
    sso.extractCredentialsFromLocalCookie(request);
    ASSERT_EQUALS(true, request.credentials()->authenticated);
    ASSERT_EQUALS("joe", request.credentials()->username);
    std::set<std::string> expectedGroups = { "foo", "bar", "baz" };
    ASSERT_EQUALS(expectedGroups, request.credentials()->groups);
    std::map<std::string, std::string> expectedMap;
    expectedMap["fullName"] = "Joe Walnes";
    expectedMap["baz"] = "bongo";
    ASSERT_EQUALS(expectedMap, request.credentials()->attributes);
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

void redirects_to_sso_server_v1() {
	SsoOptions options = SsoOptions::test();
	options.protocolVersion = SsoOptions::VERSION_1;
	options.returnPath = "/__bounceback";
	options.authServer = "https://the-auth-server:10000";
	SsoAuthenticator sso(options);

	auto response = sso.respondWithRedirectToAuthenticationServer(FakeRequest::uri("/mypage?foo").withHeader("Host", "myserver:8080"));

	ASSERT_EQUALS(ResponseCode::TemporaryRedirect, response->responseCode());
	ASSERT("Should close connection", !response->keepConnectionAlive());

	std::string expectedLocation = "https://the-auth-server:10000/login?basePath=http%3A%2F%2Fmyserver%3A8080&target=http%3A%2F%2Fmyserver%3A8080%2F%5F%5Fbounceback%3Fcontinue%3D%252Fmypage%253Ffoo&version=1";
	ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Location"));
	auto locationHeader = response->getAdditionalHeaders().find("Location")->second;
	ASSERT_EQUALS(expectedLocation, locationHeader);
}

void redirects_to_sso_server_v2() {
    SsoOptions options = SsoOptions::test();
    options.protocolVersion = SsoOptions::VERSION_2;
    options.returnPath = "/__bounceback";
    options.authServer = "https://the-auth-server:10000";
    options.requestGroups.insert("ETF MM");
    options.requestUserAttributes.insert("fullName");
    options.requestUserAttributes.insert("email");
    SsoAuthenticator sso(options);

    auto response = sso.respondWithRedirectToAuthenticationServer(FakeRequest::uri("/mypage?foo").withHeader("Host", "myserver:8080"));

    ASSERT_EQUALS(ResponseCode::TemporaryRedirect, response->responseCode());
    ASSERT("Should close connection", !response->keepConnectionAlive());

    std::string expectedLocation = "https://the-auth-server:10000/login?basePath=http%3A%2F%2Fmyserver%3A8080&target=http%3A%2F%2Fmyserver%3A8080%2F%5F%5Fbounceback%3Fcontinue%3D%252Fmypage%253Ffoo"
            "&version=2&groupsRequest=ETF%20MM&userDataRequest=email%26fullName";
    ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Location"));
    auto locationHeader = response->getAdditionalHeaders().find("Location")->second;
    ASSERT_EQUALS(expectedLocation, locationHeader);
}

void parses_bounceback_params_and_generates_redirect_v1() {
	SsoOptions options = SsoOptions::test();
	options.protocolVersion = SsoOptions::VERSION_1;
	options.localAuthKey = "I AM NOT SECURE";
	options.returnPath = "/__bounceback";
	SsoAuthenticator sso(options);

	auto response = sso.respondWithLocalCookieAndRedirectToOriginalPage(FakeRequest::uri("/__bounceback?user=joe&continue=%2fpage"));

    ASSERT_EQUALS(ResponseCode::TemporaryRedirect, response->responseCode());
    ASSERT("Should close connection", !response->keepConnectionAlive());

    ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Location"));
    auto locationHeader = response->getAdditionalHeaders().find("Location")->second;
    ASSERT_EQUALS("/page", locationHeader);

    ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Set-Cookie"));
    auto cookie = response->getAdditionalHeaders().find("Set-Cookie")->second;
    ASSERT_EQUALS("_auth=joe|49A85A78D89CE4D36997C5B48940A2C6", cookie);
}

void parses_bounceback_params_and_generates_redirect_v2() {
    SsoOptions options = SsoOptions::test();
    options.protocolVersion = SsoOptions::VERSION_2;
    options.localAuthKey = "I AM NOT SECURE";
    options.returnPath = "/__bounceback";
    SsoAuthenticator sso(options);

    auto response = sso.respondWithLocalCookieAndRedirectToOriginalPage(FakeRequest::uri("/__bounceback?user=joe&groups=tbd&userData=foo%3Dbar&continue=%2fpage"));

    ASSERT_EQUALS(ResponseCode::TemporaryRedirect, response->responseCode());
    ASSERT("Should close connection", !response->keepConnectionAlive());

    ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Location"));
    auto locationHeader = response->getAdditionalHeaders().find("Location")->second;
    ASSERT_EQUALS("/page", locationHeader);

    ASSERT_EQUALS(1, response->getAdditionalHeaders().count("Set-Cookie"));
    auto cookie = response->getAdditionalHeaders().find("Set-Cookie")->second;
    ASSERT_EQUALS("_auth=joe|tbd|foo%3Dbar|7DD1B3589E44B52B75956DD9F16E6412", cookie);
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
    RUN(extract_credentials_from_local_cookie_v1);
    RUN(extract_credentials_from_local_cookie_v2);
	RUN(parses_cookies);
    RUN(parses_bounceback_params_and_generates_redirect_v1);
    RUN(parses_bounceback_params_and_generates_redirect_v2);
	RUN(returns_same_hash_each_time);
	RUN(uses_a_random_hash);
    RUN(redirects_to_sso_server_v1);
    RUN(redirects_to_sso_server_v2);
	return TEST_REPORT();
}
 
