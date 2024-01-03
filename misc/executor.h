#ifndef RBK_EXECUTOR_H
#define RBK_EXECUTOR_H

#include <qcontainerfwd.h>
class Log;

// class Execute {
//       public:
// 	Execute(const QString& cmd);
// };

//This will internally split on space and generated the arguments
Log execute(const QString &cmd, float maxTimeInS = 999);
Log execute(QStringList& cmd, float maxTimeInS = 999);

#endif // RBK_EXECUTOR_H
