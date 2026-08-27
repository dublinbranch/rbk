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
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

class QStringAdt;

// class Execute {
//       public:
// 	Execute(const QString& cmd);
// };

//This will internally split on space and generated the arguments

struct ExecuteOpt {
	ExecuteOpt();
	static ExecuteOpt retarded();
	/** Routine sudo (mv/chown/chmod/mkdir): skip QStacker on the Log. */
	static ExecuteOpt noStackTrace();
	/** noStackTrace + inherit parent env. Use for tiny privileged helpers. */
	static ExecuteOpt routine();

	float maxTimeInS = 999;
	//for *REASON* some program write not on stdout but on stderr like nginx...
	bool                               isRetarded = false;
	/** When false, execute/sudo do not capture a stack walk (still gated by Execute_logStackTrace). */
	bool                               logStackTrace = true;
	/** When true, child inherits the parent environment (reproc extend) plus custom_env.
	 *  Default stays empty+PATH so restic/LXC callers keep a clean env. */
	bool                               inheritEnv = false;
	std::map<std::string, std::string> custom_env;
	/** Called with each stdout/stderr chunk while the process runs. Empty = drain only. */
	std::function<void(std::string_view chunk, bool isStderr)> onChunk;
};

Log execute(const QStringAdt& cmd, const ExecuteOpt& opt = {});
Log execute(const std::vector<std::string>& args, const ExecuteOpt& opt = {});

Log sudo(const QStringAdt& cmd, const ExecuteOpt& opt = {});
Log sudo(const std::vector<std::string>& args, const ExecuteOpt& opt = {});

/**
 * @brief saveInto writes content to path. Skips the write when bytes already match.
 *        If the process can write the dest (or its parent), writes directly and
 *        does not chown to root. Otherwise one `sudo install` (mode+owner).
 */
Log saveInto(const QStringAdt& path, const QByteAdt& content, QString chown = "root:root", QString chmod = "644");
//Similar but in case the file already exists
Log moveInto(const QString& old, const QString& neu, QString chown = "root:root", QString chmod = "644");
Log copyInto(const QStringAdt& old, const QStringAdt& neu, QString chown = "root:root", QString chmod = "644");

/**
 * @brief sudo-aware file existence check. Use std::filesystem::exists()
 *        for paths the current process can already read.
 *        exit_code 1 from `test -f` is treated as a valid "not found",
 *        NOT as an error in the returned Log.
 */
bool fileExists(const QStringAdt& path);

#endif // RBK_EXECUTOR_H
