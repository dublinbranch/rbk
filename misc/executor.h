#ifndef RBK_EXECUTOR_H
#define RBK_EXECUTOR_H

#include "rbk/log/log.h"
#include "rbk/string/stringoso.h"
#include <qcontainerfwd.h>

class QStringViewV2;

// class Execute {
//       public:
// 	Execute(const QString& cmd);
// };

//This will internally split on space and generated the arguments

struct ExecuteOpt {
	float maxTimeInS = 999;
	//for *REASON* some program write not on stdout but on stderr like nginx...
	bool isRetarded = false;
};

Log execute(const QString& cmd, ExecuteOpt opt = {});
Log execute(QStringList& cmd, ExecuteOpt opt = {});

Log sudo(const QStringViewV2& cmd, ExecuteOpt opt = {});

/**
 * @brief saveInto will execute sudo to move the file in place where we can not access
 * as of course we prefer to run this code not with high priviledges!
 * @param path
 * @param content
 */
Log saveInto(const QStringViewV2& path, const QByteViewV2& content, QString chown = "root:root", QString chmod = "644");
//Similar but in case the file already exists
Log moveInto(const QString& old, const QString& neu, QString chown = "root:root", QString chmod = "644");
Log copyInto(const QStringViewV2& old, const QStringViewV2& neu, QString chown = "root:root", QString chmod = "644");

#endif // RBK_EXECUTOR_H
