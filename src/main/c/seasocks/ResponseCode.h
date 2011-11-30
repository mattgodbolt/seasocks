#pragma once

#include <string>

namespace SeaSocks {

#define SEASOCKS_DEFINE_RESPONSECODE(CODE,SYMBOLICNAME,STRINGNAME) SYMBOLICNAME = CODE,
enum class ResponseCode {
#include <seasocks/ResponseCodeDefs.h>
};
#undef SEASOCKS_DEFINE_RESPONSECODE

}

const char* name(SeaSocks::ResponseCode code);
bool isOk(SeaSocks::ResponseCode code);
