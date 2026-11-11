#include "hex.h"

#include <cstddef>

namespace rbk::Hex {

namespace {

std::string encode(std::span<const u8> bytes, const char* digits) {
	std::string out(bytes.size() * 2, '0');
	for (std::size_t i = 0; i < bytes.size(); ++i) {
		out[2 * i]     = digits[bytes[i] >> 4];
		out[2 * i + 1] = digits[bytes[i] & 0x0F];
	}
	return out;
}

} // namespace

std::string upper(std::span<const u8> bytes) {
	return encode(bytes, "0123456789ABCDEF");
}

std::string lower(std::span<const u8> bytes) {
	return encode(bytes, "0123456789abcdef");
}

std::string lower(std::string_view bytes) {
	return lower(std::span{reinterpret_cast<const u8*>(bytes.data()), bytes.size()});
}

bool decode(std::string_view hex, std::span<u8> out) {
	if (hex.size() != out.size() * 2) {
		return false;
	}
	for (std::size_t i = 0; i < out.size(); ++i) {
		const int hi = digit(hex[2 * i]);
		const int lo = digit(hex[2 * i + 1]);
		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i] = static_cast<u8>((hi << 4) | lo);
	}
	return true;
}

} // namespace rbk::Hex
