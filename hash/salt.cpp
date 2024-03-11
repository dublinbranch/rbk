#include "salt.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/misc/b64.h"
#include "rbk/rand/randutil.h"
#include "sha.h"
#include <QDateTime>

std::string salt(int lenght) {
	std::string        salt;
	//LOOKS LIKE salt is NOT base64 or whatever, but MUST BE a this subset of chars
	static const char* chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static const uint  le    = (uint)strlen(chars);
	for (int i = 0; i < lenght; i++) {
		salt += chars[rand(0, le)];
	}
	return salt;
}

QString saltQS(int lenght) {
	return QString::fromStdString(salt(lenght));
}

std::string genPassword(int lenght) {
	std::string        salt;
	static const char* chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@#$%&*(){}[]!|,.;:<>?/\\";
	static const uint  le    = (uint)strlen(chars);
	for (int i = 0; i < lenght; i++) {
		salt += chars[rand(0, le)];
	}
	return salt;
}

QString genPasswordQS(int lenght) {
	return QString::fromStdString(genPassword(lenght));
}
