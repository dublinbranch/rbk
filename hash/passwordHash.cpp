#include "passwordHash.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/hash/salt.h"
#include "rbk/hash/sha.h"
#include "rbk/rand/secureRandom.h"

#include <argon2.h>
#include <crypt.h>
#include <cstring>

namespace {
constexpr uint32_t kArgonT     = 3;
constexpr uint32_t kArgonM     = 65536;
constexpr uint32_t kArgonP     = 2;
constexpr uint32_t kArgonHash  = 32;
constexpr size_t   kArgonSalt  = 16;
constexpr size_t   kEncodedMax = 512;

bool startsWith(std::string_view s, std::string_view p) {
	return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

std::string_view stripDovecotPrefix(std::string_view stored) {
	static constexpr std::string_view pref = "{SHA256-CRYPT}";
	if (startsWith(stored, pref)) {
		return stored.substr(pref.size());
	}
	return stored;
}
} // namespace

PasswordScheme detectPasswordScheme(std::string_view stored) {
	if (stored.empty()) {
		return PasswordScheme::Unknown;
	}
	if (startsWith(stored, "$argon2")) {
		return PasswordScheme::Argon2id;
	}
	auto cryptPart = stripDovecotPrefix(stored);
	if (startsWith(cryptPart, "$6$")) {
		return PasswordScheme::Sha512Crypt;
	}
	if (startsWith(cryptPart, "$5$")) {
		return PasswordScheme::Sha256Crypt;
	}
	// Legacy DiTeR panel hashes have no $ scheme prefix
	return PasswordScheme::LegacyDiterSha256;
}

QByteArray hashLegacyDiterSha256(const QString& email, const QString& password,
                                 std::string_view pepper) {
	auto       sale = salt(17);
	auto       base = F8("{}{}{}{}", sale, email, password, pepper);
	auto       sha  = sha256(base, true);
	QByteArray out;
	out.reserve(static_cast<int>(sale.size() + sha.size()));
	out.append(sale.data(), static_cast<int>(sale.size()));
	out.append(sha);
	return out;
}

bool verifyLegacyDiterSha256(const QString& email, const QString& password,
                             std::string_view pepper, std::string_view stored) {
	if (stored.size() < 17) {
		return false;
	}
	auto       sale = std::string(stored.substr(0, 17));
	auto       base = F8("{}{}{}{}", sale, email, password, pepper);
	auto       sha  = sha256(base, true);
	QByteArray expected;
	expected.reserve(17 + sha.size());
	expected.append(sale.data(), 17);
	expected.append(sha);
	return expected == QByteArray(stored.data(), static_cast<int>(stored.size()));
}

std::string hashArgon2id(std::string_view password) {
	auto saltBytes = secureRandomBytes(kArgonSalt);
	char encoded[kEncodedMax];
	const int err = argon2id_hash_encoded(
	    kArgonT, kArgonM, kArgonP,
	    password.data(), password.size(),
	    saltBytes.constData(), static_cast<size_t>(saltBytes.size()),
	    kArgonHash,
	    encoded, sizeof(encoded));
	if (err != ARGON2_OK) {
		throw ExceptionV2(F("argon2id_hash_encoded: {}", argon2_error_message(err)));
	}
	return std::string(encoded);
}

bool verifyArgon2id(std::string_view password, std::string_view encoded) {
	if (encoded.empty()) {
		return false;
	}
	// PHC string from argon2id_hash_encoded — use typed verify
	std::string enc(encoded);
	const int   err = argon2id_verify(enc.c_str(), password.data(), password.size());
	return err == ARGON2_OK;
}

namespace {
QByteArray cryptWithPrefix(std::string_view password, std::string_view idPrefix) {
	// idPrefix is "$6$" or "$5$"; crypt salt = prefix + 16 alphanumeric
	auto           sale = std::string(idPrefix) + salt(16);
	crypt_data     data;
	memset(&data, 0, sizeof(data));
	std::string    pwd(password);
	const char*    out = crypt_r(pwd.c_str(), sale.c_str(), &data);
	if (!out) {
		throw ExceptionV2("crypt_r failed");
	}
	return QByteArray(out);
}
} // namespace

QByteArray hashSha512Crypt(std::string_view password) {
	return cryptWithPrefix(password, "$6$");
}

QByteArray hashSha256Crypt(std::string_view password) {
	return cryptWithPrefix(password, "$5$");
}

QByteArray hashSha256CryptDovecot(std::string_view password) {
	auto h = hashSha256Crypt(password);
	return QByteArray("{SHA256-CRYPT}") + h;
}

bool verifyCryptPassword(std::string_view password, std::string_view stored) {
	auto           cryptPart = stripDovecotPrefix(stored);
	if (!startsWith(cryptPart, "$5$") && !startsWith(cryptPart, "$6$")) {
		return false;
	}
	crypt_data  data;
	memset(&data, 0, sizeof(data));
	std::string pwd(password);
	std::string setting(cryptPart);
	const char* out = crypt_r(pwd.c_str(), setting.c_str(), &data);
	if (!out) {
		return false;
	}
	return setting == out;
}

PasswordVerifyResult verifyPasswordAuto(const QString& email, const QString& password,
                                        std::string_view pepper, std::string_view stored) {
	PasswordVerifyResult r;
	r.scheme = detectPasswordScheme(stored);
	switch (r.scheme) {
	case PasswordScheme::Argon2id: {
		auto pwd = password.toStdString();
		r.ok     = verifyArgon2id(pwd, stored);
		break;
	}
	case PasswordScheme::Sha512Crypt:
	case PasswordScheme::Sha256Crypt: {
		auto pwd = password.toStdString();
		r.ok     = verifyCryptPassword(pwd, stored);
		break;
	}
	case PasswordScheme::LegacyDiterSha256: {
		r.ok          = verifyLegacyDiterSha256(email, password, pepper, stored);
		r.needsRehash = r.ok;
		break;
	}
	case PasswordScheme::Unknown:
		r.ok = false;
		break;
	}
	return r;
}
