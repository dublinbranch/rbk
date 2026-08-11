#include "salt.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/rand/secureRandom.h"

namespace {
// Alphanumeric only — safe in shell, SQL quotes, env files, and nginx/htpasswd paths.
// Avoid ! @ # $ & | ; < > etc. (shell/metachar breakage).
constexpr std::string_view kAlphaNum =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}

std::string salt(int lenght) {
	if (lenght <= 0) {
		throw ExceptionV2("salt: length must be > 0");
	}
	return randomString(static_cast<size_t>(lenght), kAlphaNum);
}

QString saltQS(int lenght) {
	return QString::fromStdString(salt(lenght));
}

std::string genPassword(int lenght) {
	if (lenght <= 0) {
		throw ExceptionV2("genPassword: length must be > 0");
	}
	return randomString(static_cast<size_t>(lenght), kAlphaNum);
}

QString genPasswordQS(int lenght) {
	return QString::fromStdString(genPassword(lenght));
}
