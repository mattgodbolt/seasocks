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

#include "seasocks/util/Json.h"

#include <iomanip>

namespace seasocks {

void jsonToStream(std::ostream& str, const char* t) {
    str << '"';
    for (; *t; ++t) {
        switch (*t) {
            default:
                if (static_cast<unsigned char>(*t) >= 32) {
                    str << *t;
                } else {
                    str << "\\u" << std::setw(4)
                        << std::setfill('0') << std::hex << static_cast<int>(*t);
                }
                break;
            case 8:
                str << "\\b";
                break;
            case 9:
                str << "\\t";
                break;
            case 10:
                str << "\\n";
                break;
            case 12:
                str << "\\f";
                break;
            case 13:
                str << "\\r";
                break;
            case '"':
            case '\\':
                str << '\\';
                str << *t;
                break;
        }
    }
    str << '"';
}

void jsonToStream(std::ostream& str, bool b) {
    str << (b ? "true" : "false");
}

void EpochTimeAsLocal::jsonToStream(std::ostream& o) const {
    o << "new Date(" << t * 1000 << ").toLocaleString()";
}

}
