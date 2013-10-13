// Copyright (c) 2013, Matt Godbolt
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this 
// list of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "internal/Version.h"

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"

#include <tclap/CmdLine.h>

#include <iostream>
#include <memory>

using namespace seasocks;
using namespace TCLAP;

namespace {

const char description[] = "Serve static files over HTTP.";

}

int main(int argc, const char* argv[]) {
    CmdLine cmd(description, ' ', SEASOCKS_VERSION_STRING);
    ValueArg<int> portArg("p", "port", "Listen for incoming connections on PORT", false, 80, "PORT", cmd);
    SwitchArg verboseArg("v", "verbose", "Output verbose debug information", cmd);
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
    server.serve(rootArg.getValue().c_str(), portArg.getValue());
    return 0;
}
