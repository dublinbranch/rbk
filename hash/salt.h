#ifndef SALT_H
#define SALT_H
#include <QString>
#include <string>

std::string salt(std::string, int lenght = 4);
QString     salt(QString, int lenght = 4);
QString     saltQS(int lenght = 4);

#endif // SALT_H
