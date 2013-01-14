#include "internal/Base64.h"
#include "internal/HybiAccept.h"

#include "sha1/sha1.h"

#include <arpa/inet.h>

namespace seasocks {

static const std::string magicString("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

std::string getAcceptKey(const std::string& challenge) {
	auto fullString = challenge + magicString;
	SHA1 hasher;
	hasher.Input(fullString.c_str(), fullString.size());
	unsigned hash[5];
	hasher.Result(hash);
	for (int i = 0; i < 5; ++i) {
		hash[i] = htonl(hash[i]);
	}
	return base64Encode(hash, sizeof(hash));
}

}
