#ifndef RBK_SECURE_RANDOM_H
#define RBK_SECURE_RANDOM_H

#include <QByteArray>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

/**
 * Cryptographically secure randomness (getrandom).
 * Use for passwords, salts, tokens, WP keys — NOT rbk/rand/randutil.h libc rand().
 */
void        secureRandomBytes(std::span<uint8_t> out);
QByteArray  secureRandomBytes(size_t n);
uint32_t    secureUniformIndex(uint32_t n);
std::string randomString(size_t length, std::string_view alphabet);

#endif // RBK_SECURE_RANDOM_H
