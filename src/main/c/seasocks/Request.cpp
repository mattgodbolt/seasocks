#include "seasocks/Request.h"

namespace SeaSocks {

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

}  // namespace SeaSocks
