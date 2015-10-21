#!/usr/bin/env python

import os, os.path, sys

MAX_SLICE = 70

print """
#include "internal/Embedded.h"

#include <string>
#include <unordered_map>

namespace {

std::unordered_map<std::string, EmbeddedContent> embedded = {
"""

for f in sys.argv[1:]:
	bytes = open(f, 'rb').read()
	print '{"/%s", {' % os.path.basename(f)
        for start in range(0, len(bytes), MAX_SLICE):
            print '"' + "".join(['\\x%02x' % ord(x) for x in bytes[start:start+MAX_SLICE]]) + '"'
	print ',%d }},' % len(bytes)

print """
};

}  // namespace

const EmbeddedContent* findEmbeddedContent(const std::string& name) {
	auto found = embedded.find(name);
	if (found == embedded.end()) {
		return nullptr;
	}
	return &found->second;
}
"""

