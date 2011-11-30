#include "seasocks/ResponseCode.h"

using namespace SeaSocks;

bool isOk(SeaSocks::ResponseCode code) {
	auto intValue = static_cast<int>(code);
	return intValue >= 200 && intValue < 300;
}

const char* name(ResponseCode code) {
	switch (code) {
#define SEASOCKS_DEFINE_RESPONSECODE(CODE,SYMBOLICNAME,STRINGNAME) case ResponseCode::SYMBOLICNAME: return STRINGNAME;
#include <seasocks/ResponseCodeDefs.h>
#undef SEASOCKS_DEFINE_RESPONSECODE
	}
	return "Unknown";
}
