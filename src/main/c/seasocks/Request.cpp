#include "seasocks/Request.h"

#include <cstring>

namespace seasocks {

const char* Request::name(Verb v) {
    switch(v) {
    case Invalid: return "Invalid";
    case WebSocket: return "WebSocket";
    case Get: return "Get";
    case Put: return "Put";
    case Post: return "Post";
    case Delete: return "Delete";
    default: return "???";
    }
}

Request::Verb Request::verb(const char* verb) {
    if (std::strcmp(verb, "GET") == 0) return Request::Get;
    if (std::strcmp(verb, "PUT") == 0) return Request::Put;
    if (std::strcmp(verb, "POST") == 0) return Request::Post;
    if (std::strcmp(verb, "DELETE") == 0) return Request::Delete;
    return Request::Invalid;
}

}  // namespace seasocks
