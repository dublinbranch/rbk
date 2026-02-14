/**
 * Remember to enable this lib!
 * Add in .pro file
 * WITH_REPROC = true
 * And install the dependencies (search in rbk.pri)
 */

#ifndef RBK_EXECUTOR_H
#define RBK_EXECUTOR_H

#include "rbk/log/log.h"
#include "rbk/string/stringoso.h"
#include <map>

class QStringAdt;

// class Execute {
//       public:
// 	Execute(const QString& cmd);
// };

//This will internally split on space and generated the arguments

struct ExecuteOpt {
	ExecuteOpt();
	static ExecuteOpt retarded();

	float maxTimeInS = 999;
	//for *REASON* some program write not on stdout but on stderr like nginx...
	bool                               isRetarded = false;
	std::map<std::string, std::string> custom_env;
};

Log execute(const QStringAdt& cmd, const ExecuteOpt& opt = {});
//Log execute(const std::vector<std::string>& args, ExecuteOpt opt = {});

Log sudo(const QStringAdt& cmd, const ExecuteOpt& opt = {});

/**
 * @brief saveInto will execute sudo to move the file in place where we can not access
 * as of course we prefer to run this code not with high priviledges!
 * @param path
 * @param content
 */
Log saveInto(const QStringAdt& path, const QByteAdt& content, QString chown = "root:root", QString chmod = "644");
//Similar but in case the file already exists
Log moveInto(const QString& old, const QString& neu, QString chown = "root:root", QString chmod = "644");
Log copyInto(const QStringAdt& old, const QStringAdt& neu, QString chown = "root:root", QString chmod = "644");

#endif // RBK_EXECUTOR_H
