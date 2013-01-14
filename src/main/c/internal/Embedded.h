#pragma once

#include <string>

struct EmbeddedContent {
   const char* data;
   size_t length;
};

const EmbeddedContent* findEmbeddedContent(const std::string& name);
