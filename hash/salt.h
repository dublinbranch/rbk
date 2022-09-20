#ifndef SALT_H
#define SALT_H
#include <QString>
#include <string>

std::string salt(std::string, size_t lenght = 4);
QString     salt(QString, size_t lenght = 4);
QString     saltQS(size_t lenght = 4);

#endif // SALT_H
