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

#pragma once

#include "internal/Debug.h"

// Internal stream helpers for logging.
#include <sstream>

#define LS_LOG(LOG, LEVEL, STUFF)                            \
    {                                                        \
        std::ostringstream os_;                              \
        os_ << STUFF;                                        \
        (LOG)->log(Logger::Level::LEVEL, os_.str().c_str()); \
    }

#define LS_DEBUG(LOG, STUFF) LS_LOG(LOG, Debug, STUFF)
#define LS_ACCESS(LOG, STUFF) LS_LOG(LOG, Access, STUFF)
#define LS_INFO(LOG, STUFF) LS_LOG(LOG, Info, STUFF)
#define LS_WARNING(LOG, STUFF) LS_LOG(LOG, Warning, STUFF)
#define LS_ERROR(LOG, STUFF) LS_LOG(LOG, Error, STUFF)
#define LS_SEVERE(LOG, STUFF) LS_LOG(LOG, Severe, STUFF)
