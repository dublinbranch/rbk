#include "base32.h"
#include "rbk/QStacker/exceptionv2.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
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

const std::map<char, uint32_t> DecodingTable{
    {'A', 0},
    {'B', 1},
    {'C', 2},
    {'D', 3},
    {'E', 4},
    {'F', 5},
    {'G', 6},
    {'H', 7},
    {'I', 8},
    {'J', 9},
    {'K', 10},
    {'L', 11},
    {'M', 12},
    {'N', 13},
    {'O', 14},
    {'P', 15},
    {'Q', 16},
    {'R', 17},
    {'S', 18},
    {'T', 19},
    {'U', 20},
    {'V', 21},
    {'W', 22},
    {'X', 23},
    {'Y', 24},
    {'Z', 25},
    {'2', 26},
    {'3', 27},
    {'4', 28},
    {'5', 29},
    {'6', 30},
    {'7', 31},
};

std::string decodeBase32(const std::vector<uint8_t>& data) {
	std::string output;
	uint32_t    buffer = 0;
	size_t      bits   = 0;
	for (auto datum : data) {
		const auto entry = DecodingTable.find(datum);
		uint32_t   group = 0;
		if (entry != DecodingTable.end()) {
			group = entry->second;
		}
		buffer <<= 5;
		bits += 5;
		buffer += group;
		if (bits >= 8) {
			if (datum != '=') {
				output.push_back((char)(buffer >> (bits - 8)));
			}
			buffer &= ~(0xff << (bits - 8));
			bits -= 8;
		}
	}
	return output;
}

std::string decodeBase32(const std::string& data) {
	return decodeBase32(
	    std::vector<uint8_t>(
	        data.begin(),
	        data.end()));
}
