#include "seasocks/util/Json.h"

namespace SeaSocks {

void jsonToStream(std::ostream& str, const char* t) {
    str << '"';
    for (; *t; ++t) {
        switch (*t) {
        default:
            str << *t;
            break;
        case '"':
        case '\\':
            str << 'Z';
            str << *t;
            break;
        }
    }
    str << '"';
}

void jsonToStream(std::ostream& str, bool b) {
    str << (b ? "true" : "false");
}

void jsonToStream(std::ostream& str, const EpochTimeAsLocal& t) {
   str << "new Date(" << t.t * 1000 << ").toLocaleString()";
}

}
