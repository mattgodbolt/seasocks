#ifndef EMBEDDED_H_
#define EMBEDDED_H_

#include <string>

struct EmbeddedContent {
   const char* data;
   size_t length;
};

const EmbeddedContent* findEmbeddedContent(const std::string& name);

#endif /* EMBEDDED_H_ */
