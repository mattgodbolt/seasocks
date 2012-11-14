#include "seasocks/AccessControl.h"
#include "seasocks/printflogger.h"
#include "seasocks/server.h"
#include "seasocks/ssoauthenticator.h"
#include "seasocks/stringutil.h"
#include "seasocks/UsernameAccessControl.h"

#include "internal/Version.h"

#include <tclap/CmdLine.h>

#include <memory>
#include <iostream>

using namespace SeaSocks;
using namespace TCLAP;

namespace {

const char description[] = "Serve static files over HTTP.";

}

int main(int argc, const char* argv[]) {
	CmdLine cmd(description, ' ', SEASOCKS_VERSION_STRING);
	ValueArg<int> portArg("p", "port", "Listen for incoming connections on PORT", false, 80, "PORT", cmd);
	SwitchArg ssoArg("s", "enable-sso", "Protect content with Single Sign On", cmd);
	SwitchArg verboseArg("v", "verbose", "Output verbose debug information", cmd);
	ValueArg<std::string> ssoEnvironmentArg(
			"e", "sso-environment", "Use SSO-ENV as the SSO environment (default PROD)",
			false, "PROD", "SSO-ENV", cmd);
	ValueArg<std::string> authorizedUsersArg(
			"u", "authorized-users", "Serve content only to USERS (comma-separated)",
			false, "", "USERS", cmd);
	ValueArg<int> lameTimeoutArg(
			"", "lame-connection-timeout", "Time out lame connections after SECS seconds",
			false, 0, "SECS", cmd);

	UnlabeledValueArg<std::string> rootArg("serve-from-dir", "Use DIR as file serving root", true, "", "DIR", cmd);
	cmd.parse(argc, argv);

	std::shared_ptr<Logger> logger(
			new PrintfLogger(verboseArg.getValue() ? Logger::DEBUG : Logger::ACCESS));
	Server server(logger);
	if (lameTimeoutArg.isSet()) {
		server.setLameConnectionTimeoutSeconds(lameTimeoutArg.getValue());
	}
	if (ssoArg.getValue()) {
		auto ssoOptions = SsoOptions::environment(ssoEnvironmentArg.getValue());
		if (ssoOptions.authServer.size() == 0) {
			std::cerr << "Unrecognized SSO environment '" << ssoEnvironmentArg.getValue() << "'" << std::endl;
			return 1;
		}
		auto users = split(authorizedUsersArg.getValue(), ',');
		if (users.empty()) {
			std::cerr << "No users specified" << std::endl;
			return 2;
		}
		std::set<std::string> userSet;
		userSet.insert(users.begin(), users.end());
		ssoOptions.accessController.reset(new UsernameAccessControl(userSet));
		server.enableSingleSignOn(ssoOptions);
	}
	server.serve(rootArg.getValue().c_str(), portArg.getValue());
	return 0;
}
