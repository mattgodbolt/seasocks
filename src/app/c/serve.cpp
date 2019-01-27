// Copyright (c) 2013-2017, Matt Godbolt
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

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"

#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <memory>

using namespace seasocks;

namespace {

const char usage[] = "Usage: %s [-p PORT] [-v] DIR\n"
                     "   Serves files from DIR over HTTP on port PORT\n";

}

int main(int argc, char* const argv[]) {
    int port = 80;
    bool verbose = false;
    int opt;
    while ((opt = getopt(argc, argv, "vp:")) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            default:
                fprintf(stderr, usage, argv[0]);
                exit(1);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    auto logger = std::make_shared<PrintfLogger>(
        verbose ? Logger::Level::Debug : Logger::Level::Access);
    Server server(logger);
    auto root = argv[optind];
    server.serve(root, port);
    return 0;
}
