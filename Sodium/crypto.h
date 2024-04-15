#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SODIUM_CRYPTO_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SODIUM_CRYPTO_H
#include <optional>
#include <string>

std::string                encode(const std::string& message, const std::string& key);
std::optional<std::string> decode(const std::string& message, const std::string& key);
#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SODIUM_CRYPTO_H
