#include "base32.h"
#include <cstdint>
#include <string>

const char base32Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

//TODO use the #include <cryptopp/base32.h> and make a wrapper, as they are the most un ergonomic function ever

std::string encodeBase32(const std::string& data) {
	std::string encoded;
	int         value = 0;
	int         bits  = 0;
	for (uint8_t byte : data) {
		value = (value << 8) | byte;
		bits += 8;

		while (bits >= 5) {
			encoded += base32Alphabet[(value >> (bits - 5)) & 31];
			bits -= 5;
		}
	}
	if (bits > 0) {
		encoded += base32Alphabet[(value << (5 - bits)) & 31];
	}
	return encoded;
}
