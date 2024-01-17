#ifndef FOLDER_H
#define FOLDER_H

#include "ffCommon.h"
#include "rbk/string/stringoso.h"
#include <QString>

namespace RBK {
//Inside namespace to avoid clash with the standard posix function
bool mkdir(const std::string& dirName);
bool mkdir(const char* dirName);
} // namespace RBK

bool mkdir(const QString& dirName);

void        cleanFolder(const QString& folder);
QString     getMostRecent(const QString pathDir, const QString& filter);
QStringList search(const QString& path);
uint        erase(const QStringList& files);

QString hardLinkFolder(const QString& source, const QString& dest, HLParam param = HLParam::eraseOld);

bool rmdirV2(const QStringV2& path);
#endif // FOLDER_H
