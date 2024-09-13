#pragma once

#include <string>

unsigned int generate_totp(const std::string& secret_key, unsigned long time_step = 30);
unsigned int generateTotpAt(const std::string& secret_key, unsigned long timeSTEP);
