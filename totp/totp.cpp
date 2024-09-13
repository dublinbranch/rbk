#include "totp.h"
#include "rbk/misc/base32.h"
#include <cmath>
#include <ctime>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// TOTP generation function
unsigned int generate_totp(const std::string& secret_key, unsigned long stepSize) {
	// Get current time step
	unsigned long current_time = (std::time(nullptr) / stepSize);
	return generateTotpAt(secret_key, current_time);
}

unsigned int generateTotpAt(const std::string& secret_key, unsigned long timeSTEP) {

	// Convert time step to a byte array (big-endian)
	unsigned char time_bytes[8];
	for (int i = 7; i >= 0; --i) {
		time_bytes[i] = timeSTEP & 0xFF;
		timeSTEP >>= 8;
	}

	// Decode the base32 secret key
	std::string key = decodeBase32(secret_key);

	// Create HMAC-SHA1 hash
	unsigned char* hash = HMAC(EVP_sha1(), key.c_str(), (int)key.size(), time_bytes, sizeof(time_bytes), nullptr, nullptr);

	// Extract the dynamic offset from the last nibble of the hash
	int offset = hash[19] & 0x0F;

	// Extract 4 bytes starting from the dynamic offset
	unsigned int binary_code = ((hash[offset] & 0x7F) << 24) |
	                           ((hash[offset + 1] & 0xFF) << 16) |
	                           ((hash[offset + 2] & 0xFF) << 8) |
	                           (hash[offset + 3] & 0xFF);

	// Compute the 6-digit TOTP code
	unsigned int otp = binary_code % static_cast<int>(std::pow(10, 6));
	//echo("Generated TOTP: {:06} \n", totp);
	return otp;
}
