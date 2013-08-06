#include "seasocks/ResponseCode.h"

using namespace seasocks;

bool isOk(seasocks::ResponseCode code) {
    return static_cast<int>(code) < 400;
}

const char* name(ResponseCode code) {
    switch (code) {
#define SEASOCKS_DEFINE_RESPONSECODE(CODE,SYMBOLICNAME,STRINGNAME) case ResponseCode::SYMBOLICNAME: return STRINGNAME;
#include "seasocks/ResponseCodeDefs.h"

#undef SEASOCKS_DEFINE_RESPONSECODE
    }
    return "Unknown";
}
