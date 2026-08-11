#include "secureRandom.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/fmtExtra/includeMe.h"

#include <cerrno>
#include <cstdint>
#include <sys/random.h>

void secureRandomBytes(std::span<uint8_t> out) {
	if (out.empty()) {
		return;
	}
	size_t filled = 0;
	while (filled < out.size()) {
		const auto n = getrandom(out.data() + filled, out.size() - filled, 0);
		if (n < 0) {
			throw ExceptionV2(F("getrandom failed: errno {}", errno));
		}
		if (n == 0) {
			throw ExceptionV2("getrandom returned 0 bytes");
		}
		filled += static_cast<size_t>(n);
	}
}

QByteArray secureRandomBytes(size_t n) {
	QByteArray buf(static_cast<int>(n), Qt::Uninitialized);
	secureRandomBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(buf.data()), n));
	return buf;
}

uint32_t secureUniformIndex(uint32_t n) {
	if (n == 0) {
		throw ExceptionV2("secureUniformIndex: n == 0");
	}
	if (n == 1) {
		return 0;
	}
	// rejection sampling — avoid modulo bias
	const uint32_t limit = (UINT32_MAX / n) * n;
	uint32_t       x     = 0;
	for (;;) {
		secureRandomBytes(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&x), sizeof(x)));
		if (x < limit) {
			return x % n;
		}
	}
}

std::string randomString(size_t length, std::string_view alphabet) {
	if (length == 0) {
		throw ExceptionV2("randomString: length == 0");
	}
	if (alphabet.empty()) {
		throw ExceptionV2("randomString: empty alphabet");
	}
	std::string out;
	out.resize(length);
	const auto n = static_cast<uint32_t>(alphabet.size());
	for (size_t i = 0; i < length; ++i) {
		out[i] = alphabet[secureUniformIndex(n)];
	}
	return out;
}
