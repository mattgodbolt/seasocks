#!/usr/bin/env python

import os, os.path, sys

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
	print '"' + "".join(['\\x%02x' % ord(x) for x in bytes]) + '"'
	print ',%d }},' % len(bytes)

print """
};

}  // namespace

const EmbeddedContent* findEmbeddedContent(const std::string& name) {
	auto found = embedded.find(name);
	if (found == embedded.end()) {
		return NULL;
	}
	return &found->second;
}
"""

