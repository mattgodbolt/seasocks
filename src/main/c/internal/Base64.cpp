#include "internal/Base64.h"

#include <cstdint>

namespace SeaSocks {

const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const void* dataVoid, size_t length) {
	std::string output;
	auto data = reinterpret_cast<const uint8_t*>(dataVoid);
	for (auto i = 0u; i < length; i += 3) {
		auto bytesLeft = length - i;
		auto b0 = data[i];
		auto b1 = bytesLeft > 1 ? data[i + 1] : 0;
		auto b2 = bytesLeft > 2 ? data[i + 2] : 0;
		output.push_back(cb64[b0 >> 2]);
		output.push_back(cb64[((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4)]);
		output.push_back((bytesLeft > 1 ? cb64[((b1 & 0x0f) << 2) | ((b2 & 0xc0) >> 6)] : '='));
		output.push_back((bytesLeft > 2 ? cb64[b2 & 0x3f] : '='));
	}
	return output;
}

} // namespace SeaSocks
