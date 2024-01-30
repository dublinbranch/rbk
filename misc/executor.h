#ifndef RBK_EXECUTOR_H
#define RBK_EXECUTOR_H

#include <qcontainerfwd.h>
class Log;
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
#endif // RBK_EXECUTOR_H
