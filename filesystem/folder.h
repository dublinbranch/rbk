#ifndef FOLDER_H
#define FOLDER_H

#include "ffCommon.h"
#include <QString>

bool        mkdir(const QString& dirName);
void        cleanFolder(const QString& folder);
QString     getMostRecent(const QString pathDir, const QString& filter);
QStringList search(const QString& path);
uint        erase(const QStringList& files);

QString hardLinkFolder(const QString& source, const QString& dest, HLParam param = HLParam::eraseOld);

#endif // FOLDER_H
