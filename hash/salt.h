#ifndef SALT_H
#define SALT_H
#include <QString>
#include <string>

std::string salt(int lenght = 4);
std::string genPassword(int lenght = 16);
QString     genPasswordQS(int lenght = 16);
QString     saltQS(int lenght = 4);

#endif // SALT_H
