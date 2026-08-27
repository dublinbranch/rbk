#include "executor.h"

#include "echo.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/log/log.h"
#include <QDateTime>
#include <QDebug>

#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>
#include <unistd.h>

using namespace std;

thread_local bool Execute_logStackTrace = true;
thread_local bool Execute_printOnError  = false;

ExecuteOpt::ExecuteOpt() {
	//pretty sure you always want that...
	custom_env = {{"PATH", "/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"}};
}

ExecuteOpt ExecuteOpt::retarded() {
	ExecuteOpt opt;
	opt.isRetarded = true;
	return opt;
}

ExecuteOpt ExecuteOpt::noStackTrace() {
	ExecuteOpt opt;
	opt.logStackTrace = false;
	return opt;
}

namespace {

QString joinArgv(const std::vector<std::string>& args) {
	QString out;
	for (size_t i = 0; i < args.size(); ++i) {
		if (i) {
			out += QLatin1Char(' ');
		}
		out += QString::fromStdString(args[i]);
	}
	return out;
}

Log executeImpl(const std::vector<std::string>& ar, const QString& section, const ExecuteOpt& opt) {
	Log log;
	log.section  = "execute: " + section;
	log.options  = F("maxTime: {} env: {} ", opt.maxTimeInS, opt.custom_env);
	log.category = Log::Exception;

	reproc::process process;
	reproc::options options;
	options.env.behavior      = reproc::env::empty;
	options.env.extra         = opt.custom_env;
	options.redirect.parent   = false;
	options.redirect.err.type = reproc::redirect::pipe;
	options.redirect.out.type = reproc::redirect::pipe;

	if (opt.maxTimeInS > 0) {
		options.deadline = reproc::milliseconds((int)(opt.maxTimeInS * 1000));
	}

	std::error_code ec = process.start(ar, options);

	//we are not going to write any data, plus if you do not do this LXC will hang!!!
	process.close(reproc::stream::in);

	if (ec == std::errc::no_such_file_or_directory) {
		log.category = Log::Error;
		throw ExceptionV2(F("Program >>>{}<<< not found. Make sure it's available from the PATH.", section));
	}
	if (ec) {
		log.category = Log::Error;
		log.stdErr   = F8("{} {}", ec.message(), ec.value());
		return log;
	}
	std::string output;
	std::string error;

	struct ChunkSink {
		std::string*                                       acc = nullptr;
		const std::function<void(std::string_view, bool)>* cb  = nullptr;
		bool                                               err = false;
		std::error_code operator()(reproc::stream, const uint8_t* buffer, unsigned int size) {
			std::string_view sv(reinterpret_cast<const char*>(buffer), static_cast<size_t>(size));
			if (acc) {
				acc->append(sv);
			}
			if (cb && *cb) {
				(*cb)(sv, err);
			}
			return {};
		}
	};

	if (opt.onChunk) {
		ChunkSink sinkOut{&output, &opt.onChunk, false};
		ChunkSink sinkErr{&error, &opt.onChunk, true};
		ec = reproc::drain(process, sinkOut, sinkErr);
	} else {
		reproc::sink::string sink(output);
		reproc::sink::string sinkErr(error);
		ec = reproc::drain(process, sink, sinkErr);
	}

	{
		// Assicura che il processo sia terminato e recupera exit code
		auto [exit_code, ec_wait] = process.wait(reproc::infinite);
		log.exit_code             = exit_code;
		log.ec_wait               = ec_wait;
	}

	if (!output.empty()) {
		log.stdOut = QByteArray::fromStdString(output);
	}

	if (ec) {
		log.stdErr.append(F8("\n{} {}\n", ec.message(), ec.value()));
	}

	if (!error.empty()) {
		log.stdErr.append(QByteArray::fromStdString(error));
	}

	log.setEnd();

	if (Execute_logStackTrace && opt.logStackTrace) {
		log.stackTrace = QStacker();
	}

	if (opt.isRetarded) {
		//if they are retarded and put normal log into stderr we can not do much...
		log.stdOut += log.stdErr;
		log.stdErr.clear();
		log.category = Log::Info;
	} else {
		if (log.stdErr.isEmpty()) {
			//at this stage I do not know if a program output is info or warning here
			log.category = Log::Info;
		} else {
			log.category = Log::Error;
			if (Execute_printOnError) {
				qCritical().noquote() << F16("For {} \n stdlog: {}\n stderr: {} \n Trace: {}",
				                             section, log.stdOut, log.stdErr, log.stackTrace);
			}
		}
	}

	if (log.ec_wait) {
		log.stdErr.append(F8("\nwait: {} {}\n", log.ec_wait.message(), log.ec_wait.value()));
	}

	if (log.exit_code != 0) {
		log.category = Log::Error;
		log.stdErr.append(F8("\nexit_code: {}\n", log.exit_code));
	}

	return log;
}

} // namespace

Log execute(const QStringAdt& args, const ExecuteOpt& opt) {
	return executeImpl({"/bin/bash", "-c", args.toStdString()}, args, opt);
}

Log execute(const std::vector<std::string>& args, const ExecuteOpt& opt) {
	if (args.empty()) {
		Log log;
		log.section   = "execute: <empty argv>";
		log.category  = Log::Error;
		log.exit_code = 1;
		log.stdErr    = "empty argv";
		return log;
	}
	return executeImpl(args, joinArgv(args), opt);
}

Log sudo(const QStringAdt& cmd, const ExecuteOpt& opt) {
	if (geteuid() == 0) {
		return execute(cmd, opt);
	}
	return execute("sudo " + cmd, opt);
}

Log sudo(const std::vector<std::string>& args, const ExecuteOpt& opt) {
	if (geteuid() == 0) {
		return execute(args, opt);
	}
	std::vector<std::string> sudoArgs;
	sudoArgs.reserve(args.size() + 1);
	sudoArgs.emplace_back("sudo");
	sudoArgs.insert(sudoArgs.end(), args.begin(), args.end());
	return execute(sudoArgs, opt);
}

Log saveInto(const QStringAdt& path, const QByteAdt& content, QString chown, QString chmod) {
	Log log;
	log.section  = "saveInto";
	log.category = Log::Exception;

	auto temp = getTempFile(QString{});

	filePutContents(content, temp, true);
	const auto noTrace = ExecuteOpt::noStackTrace();
	log.push(sudo(std::vector<std::string>{"mv", QStringAdt(temp).toStdString(), QStringAdt(path).toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chown", chown.toStdString(), QStringAdt(path).toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chmod", chmod.toStdString(), QStringAdt(path).toStdString()}, noTrace));

	log.category = Log::Info;
	return log;
}

Log moveInto(const QString& old, const QString& neu, QString chown, QString chmod) {
	Log log;
	log.section  = __PRETTY_FUNCTION__;
	log.category = Log::Exception;

	const auto noTrace = ExecuteOpt::noStackTrace();
	log.push(sudo(std::vector<std::string>{"mv", old.toStdString(), neu.toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chown", chown.toStdString(), neu.toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chmod", chmod.toStdString(), neu.toStdString()}, noTrace));

	log.category = Log::Info;
	return log;
}

Log copyInto(const QStringAdt& old, const QStringAdt& neu, QString chown, QString chmod) {
	Log log;
	log.section  = __PRETTY_FUNCTION__;
	log.category = Log::Exception;

	const auto noTrace = ExecuteOpt::noStackTrace();
	log.push(sudo(std::vector<std::string>{"cp", QStringAdt(old).toStdString(), QStringAdt(neu).toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chown", chown.toStdString(), QStringAdt(neu).toStdString()}, noTrace));
	log.push(sudo(std::vector<std::string>{"chmod", chmod.toStdString(), QStringAdt(neu).toStdString()}, noTrace));

	log.category = Log::Info;
	return log;
}

bool fileExists(const QStringAdt& path) {

	auto log    = sudo(std::vector<std::string>{"test", "-f", QStringAdt(path).toStdString()}, ExecuteOpt());
	log.section = F16("fileExists: {}", path);
	log.used    = true;

	switch (log.exit_code) {
	case 0:
		return true;
	case 1:
		return false;
	default:
		throw log;
	}
}
