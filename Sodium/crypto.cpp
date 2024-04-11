#include "crypto.h"
#include "rbk/QStacker/exceptionv2.h"
#include <sodium/crypto_secretbox.h>
#include <sodium/randombytes.h>

std::string encode(const std::string& message, const std::string& key) {
	std::string nonce(crypto_secretbox_NONCEBYTES, 0);
	randombytes_buf(&nonce[0], nonce.size());

	std::string ciphertext(crypto_secretbox_MACBYTES + message.size(), 0);

	crypto_secretbox_easy(reinterpret_cast<unsigned char*>(&ciphertext[0]),
	                      reinterpret_cast<const unsigned char*>(message.data()),
	                      message.size(),
	                      reinterpret_cast<const unsigned char*>(nonce.data()),
	                      reinterpret_cast<const unsigned char*>(key.data()));

	return nonce + ciphertext;
}

std::string decode(const std::string& message, const std::string& key) {
	if (message.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
		throw ExceptionV2("message is too short");
	}

	std::string plaintext(message.size() - crypto_secretbox_NONCEBYTES - crypto_secretbox_MACBYTES, 0);

	if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char*>(&plaintext[0]),
	                               reinterpret_cast<const unsigned char*>(message.data() + crypto_secretbox_NONCEBYTES),
	                               message.size() - crypto_secretbox_NONCEBYTES,
	                               reinterpret_cast<const unsigned char*>(message.data()),
	                               reinterpret_cast<const unsigned char*>(key.data())) != 0) {
		throw ExceptionV2("decryption failed");
	}

	return plaintext;
}
