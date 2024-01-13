#include "executor.h"

#include "rbk/log/log.h"
#include <QDateTime>
#include <QDebug>
#include <QProcess>

#include "config.h"
#include "rbk/QStacker/qstacker.h"

using namespace std;

Log execute(QStringList& cmd, float maxTimeInS) {
	Log      log;
	QProcess process;
	auto     env = QProcessEnvironment::systemEnvironment();
	env.remove("LD_LIBRARY_PATH");
	process.setProcessEnvironment(env);
	auto task = cmd.takeFirst();
	process.start(task, cmd);
	process.waitForFinished(static_cast<int>(maxTimeInS) * 1000);
	log.setEnd();

	log.section = task;
	log.stdErr  = process.readAllStandardError();
	log.stdOut  = process.readAllStandardOutput();
	if (log.stdErr.isEmpty()) {
		//at this stage I do not know if a program output is info or warning here
		log.category = Log::Info;
	} else {
		log.category = Log::Error;
	}

	if (conf().devel.logStackTrace) {
		log.stackTrace = QStacker();
	}

	return log;
}

Log execute(const QString& cmd, float maxTimeInS) {
	auto param = cmd.split(" ");
	return execute(param, maxTimeInS);
}

Log sudo(const QString& cmd, float maxTimeInS) {
	QStringList pack;
	pack << "/bin/bash";
	pack << "-c"
	     << "sudo " + cmd;
	return execute(pack, maxTimeInS);
}
