// Copyright (c) 2013-2026, Matt Godbolt and Nguyen Tran
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

#include "internal/AcceptEncoding.h"

#include "seasocks/StringUtil.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace seasocks {

namespace {

// the qvalue of an Accept-Encoding element, defaulting to 1 when the weight is
// absent (RFC 9110 section 12.4.2) or unparseable
double weightOf(const std::string& element, std::string::size_type separator) {
    if (separator == std::string::npos) {
        return 1.0;
    }
    auto weight = trimWhitespace(element.substr(separator + 1));
    if (!caseInsensitiveSame(weight.substr(0, 2), "q=")) {
        return 1.0;
    }
    try {
        auto parsedWeight = std::stod(weight.substr(2));
        // stod accepts "nan"/"inf" without throwing; treat those as unparseable too
        return std::isfinite(parsedWeight) ? parsedWeight : 1.0;
    } catch (const std::logic_error&) {
        return 1.0;
    }
}

} // namespace

bool acceptsGzip(const std::string& acceptEncoding) {
    // sentinel for a coding that is not in the header; valid qvalues are 0..1, never -1
    constexpr double NotListed = -1.0;
    double gzipWeight = NotListed;
    double wildcardWeight = NotListed;
    for (const auto& entry : split(acceptEncoding, ',')) {
        auto semicolon = entry.find(';');
        auto coding = trimWhitespace(entry.substr(0, semicolon));
        if (caseInsensitiveSame(coding, "gzip")) {
            gzipWeight = weightOf(entry, semicolon);
        } else if (caseInsensitiveSame(coding, "*")) {
            wildcardWeight = weightOf(entry, semicolon);
        }
    }
    // use gzip's own weight if it is listed; otherwise fall back to the wildcard
    if (gzipWeight != NotListed) {
        return gzipWeight > 0.0;
    }
    return wildcardWeight > 0.0;
}

}
