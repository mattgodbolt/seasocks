#pragma once

#include <string>

namespace seasocks {

#define SEASOCKS_DEFINE_RESPONSECODE(CODE,SYMBOLICNAME,STRINGNAME) SYMBOLICNAME = CODE,
enum class ResponseCode {
#include <seasocks/ResponseCodeDefs.h>
};
#undef SEASOCKS_DEFINE_RESPONSECODE

}

const char* name(seasocks::ResponseCode code);
bool isOk(seasocks::ResponseCode code);
