#include "seasocks/printflogger.h"
#include "seasocks/server.h"
#include "seasocks/ssoauthenticator.h"

#include "internal/Version.h"

#include <tclap/CmdLine.h>

#include <boost/shared_ptr.hpp>
#include <iostream>

using namespace SeaSocks;
using namespace TCLAP;

static const char description[] =
		"Serve static files over HTTP.";

int main(int argc, const char* argv[]) {
	CmdLine cmd(description, ' ', SEASOCKS_VERSION_STRING);
	ValueArg<int> portArg("p", "port", "Listen for incoming connections on PORT", false, 80, "PORT", cmd);
	SwitchArg ssoArg("s", "enable-sso", "Protect content with Single Sign On", cmd);
	SwitchArg verboseArg("v", "verbose", "Output verbose debug information", cmd);
	ValueArg<std::string> ssoEnvironmentArg(
			"e", "sso-environment", "Use SSO-ENV as the SSO environment (default PROD)",
			false, "PROD", "SSO-ENV", cmd);

	UnlabeledValueArg<std::string> rootArg("serve-from-dir", "Use DIR as file serving root", true, "", "DIR", cmd);
	cmd.parse(argc, argv);

	boost::shared_ptr<Logger> logger(
			new PrintfLogger(verboseArg.getValue() ? Logger::Level::DEBUG : Logger::Level::INFO));
	Server server(logger);
	if (ssoArg.getValue()) {
		auto ssoOptions = SsoOptions::environment(ssoEnvironmentArg.getValue());
		if (ssoOptions.authServer.size() == 0) {
			std::cerr << "Unrecognized SSO environment '" << ssoEnvironmentArg.getValue() << "'" << std::endl;
			return 1;
		}
		server.enableSingleSignOn(ssoOptions);
	}
	server.serve(rootArg.getValue().c_str(), portArg.getValue());
	return 0;
}
