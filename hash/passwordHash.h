#ifndef RBK_PASSWORD_HASH_H
#define RBK_PASSWORD_HASH_H

#include <QByteArray>
#include <QString>
#include <string>
#include <string_view>

/**
 * Password hashing with scheme detection (PHC / modular crypt $…$ prefixes).
 *
 * - Legacy DiTeR panel: salt(17) || urlSafeBase64(sha256(salt||email||password||pepper)) — no $
 * - Argon2id (panel + FTP): PHC string from argon2id_hash_encoded — $argon2id$v=19$…
 * - $6$ / $5$ helpers for htpasswd / Dovecot (nginx does not speak Argon2)
 */
enum class PasswordScheme {
	LegacyDiterSha256,
	Argon2id,
	Sha256Crypt,
	Sha512Crypt,
	Unknown
};

PasswordScheme detectPasswordScheme(std::string_view stored);

// --- Legacy DiTeR (verify / optional re-hash of old rows only) ---
bool       verifyLegacyDiterSha256(const QString& email, const QString& password,
                                   std::string_view pepper, std::string_view stored);
QByteArray hashLegacyDiterSha256(const QString& email, const QString& password,
                                 std::string_view pepper);

// --- Argon2id (PHC encoded; t=3, m=65536, p=2, hashlen=32 — match DiTeR FTP argonizza) ---
std::string hashArgon2id(std::string_view password);
bool        verifyArgon2id(std::string_view password, std::string_view encoded);

// --- crypt_r helpers (htpasswd / mail) ---
QByteArray hashSha512Crypt(std::string_view password);        // $6$…
QByteArray hashSha256Crypt(std::string_view password);        // $5$…
QByteArray hashSha256CryptDovecot(std::string_view password); // {SHA256-CRYPT}$5$…
bool       verifyCryptPassword(std::string_view password, std::string_view stored);

struct PasswordVerifyResult {
	bool           ok          = false;
	bool           needsRehash = false;
	PasswordScheme scheme      = PasswordScheme::Unknown;
};

/**
 * Panel verify: Argon2id if PHC, else legacy sha256+pepper.
 * needsRehash is set when ok && scheme == LegacyDiterSha256.
 */
PasswordVerifyResult verifyPasswordAuto(const QString& email, const QString& password,
                                        std::string_view pepper, std::string_view stored);

#endif // RBK_PASSWORD_HASH_H
