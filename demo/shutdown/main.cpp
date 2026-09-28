// Demo and test bed for rbk::Shutdown. See README.md and demo.sh next to this file.
#include "rbk/HTTP/Payload.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/beast.h"
#include "rbk/misc/QDebugHandler.h"
#include "rbk/thread/shutdown.h"
#include "rbk/thread/threadvector.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QProcess>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <fmt/format.h>

#include <atomic>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>

// Beast's thread monitor reads this app-defined global. The demo has no database.
class DB;
DB* mainDB = nullptr;

namespace Sd = rbk::Shutdown;
using namespace std::chrono_literals;

namespace {
// Shared with the routes, which are plain function pointers.
std::mutex       jobMutex;
std::deque<int>  jobs; // duration of each job in ms
Sd::Wakeup       jobWake;
std::atomic<int> jobsDone{0};
ushort           httpPort = 8097;

void ticker() {
	Sd::Loop guard{"ticker"};
	int      n = 0;
	// The long sleep is on purpose: shutdown must cut it short.
	while (Sd::sleepFor(30s)) {
		qInfo("ticker: tick %d", ++n);
	}
}

void worker() {
	Sd::Loop guard{"worker"};
	while (Sd::running()) {
		std::optional<int> ms;
		{
			std::lock_guard lock(jobMutex);
			if (!jobs.empty()) {
				ms = jobs.front();
				jobs.pop_front();
			}
		}
		if (!ms) {
			if (jobWake.waitFor(1h) == Sd::Wakeup::Result::stopping) {
				return;
			}
			continue;
		}
		// Plain, uninterruptible work: shows that the job in hand finishes.
		std::this_thread::sleep_for(std::chrono::milliseconds(*ms));
		qInfo("worker: job %d done", jobsDone.fetch_add(1) + 1);
	}
}

void timer() {
	Sd::Loop                  guard{"timer"};
	boost::asio::io_context   ioc;
	boost::asio::steady_timer t(ioc);

	std::function<void()> arm;
	arm = [&] {
		t.expires_after(10s);
		t.async_wait([&](const boost::system::error_code& ec) {
			if (ec) {
				return;
			}
			qInfo("timer: fired");
			arm();
		});
	};
	arm();

	auto reg = Sd::stopOnShutdown(ioc);
	ioc.run();
}

void stuck() {
	Sd::Loop guard{"stuck"};
	std::this_thread::sleep_for(1h); // ignores shutdown on purpose
}

void json(Payload& payload, std::string body) {
	payload.mime       = "application/json";
	payload.html       = std::move(body);
	payload.statusCode = 200;
}

void routeStatus(PMFCGI&, Payload& payload) {
	json(payload, fmt::format(R"({{"running": {}, "jobsDone": {}}})", Sd::running(), jobsDone.load()));
}

void routeJob(PMFCGI& status, Payload& payload) {
	auto   ms = status.get.get("ms", 200);
	size_t n  = 0;
	{
		std::lock_guard lock(jobMutex);
		jobs.push_back(ms);
		n = jobs.size();
	}
	jobWake.notify();
	json(payload, fmt::format(R"({{"queued": {}}})", n));
}

void routeExit(PMFCGI&, Payload& payload) {
	Sd::request();
	json(payload, R"({"ok": true})");
}

void http() {
	Sd::Loop guard{"http"};
	Beast    b;
	b.conf.worker    = 2;
	b.conf.port      = httpPort;
	b.conf.logFolder = "httpLog";
	b.conf.prePhase1 = [](PMFCGI& status, Payload&) { status.decodeGet(); };
	auto& r          = b.conf.routingSimple;
	r.insert({"", routeStatus});
	r.insert({"job", routeJob});
	r.insert({"exit", routeExit});
	b.listen();
	qInfo("http: listen returned");
}

// SigBlk of a process, as the hex string in /proc/<pid>/status.
std::string sigBlk(qint64 pid) {
	std::ifstream status(fmt::format("/proc/{}/status", pid));
	std::string   line;
	while (std::getline(status, line)) {
		if (line.starts_with("SigBlk:")) {
			auto pos = line.find_first_not_of(" \t", 7);
			return pos == std::string::npos ? "" : line.substr(pos);
		}
	}
	return "?";
}

void startChild(QProcess& p, const char* kind) {
	p.start("sleep", {"600"});
	if (!p.waitForStarted()) {
		qCritical("child: %s did not start", kind);
		return;
	}
	qInfo("child: %s pid=%lld sigblk=%s", kind, p.processId(), sigBlk(p.processId()).c_str());
}
} // namespace

int main(int argc, char* argv[]) {
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationName("rbk_shutdown_demo");

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addOption({"port", "HTTP port on 127.0.0.1", "N", "8097"});
	parser.addOption({"deadline", "install() deadline in seconds", "S", "3"});
	parser.addOption({"no-install", "skip install(), to show the old behaviour"});
	parser.addOption({"stuck", "also start a thread that ignores shutdown"});
	parser.addOption({"stuck-callback", "register an onStop callback that blocks forever"});
	parser.addOption({"slow-start", "sleep S seconds after install(), before any thread", "S", "0"});
	parser.addOption({"child", "spawn sleep 600 with a plain QProcess"});
	parser.process(app);

	httpPort = parser.value("port").toUShort();

	// Pattern rule 1: right after QCoreApplication, before any thread.
	if (!parser.isSet("no-install")) {
		bool ok = Sd::install(std::chrono::seconds(parser.value("deadline").toInt()));
		qInfo("main: install -> %s", ok ? "true" : "false");
	}

	NanoSpammerConfig spamConf;
	spamConf.warningToMail = false; // else every qWarning tries to send a mail
	spamConf.startedAt     = QDateTime::currentDateTimeUtc();
	commonInitialization(&spamConf);

	if (auto s = parser.value("slow-start").toInt(); s > 0) {
		std::this_thread::sleep_for(std::chrono::seconds(s)); // a signal during startup
	}

	// Destroyed at the end of main: QProcess kills a child that still runs.
	QProcess child;
	if (parser.isSet("child")) {
		startChild(child, "QProcess");
	}

	if (parser.isSet("stuck-callback")) {
		// Breaks the onStop rule on purpose: the watcher blocks, the alarm backstop fires.
		static auto reg = Sd::onStop([] {
			for (;;) {
				pause();
			}
		});
	}

	ThreadVector tv;
	tv.emplace_back(ticker);
	tv.emplace_back(worker);
	tv.emplace_back(timer);
	tv.emplace_back(http);
	if (parser.isSet("stuck")) {
		tv.emplace_back(stuck);
	}
	qInfo("main: ready");

	app.exec();
	qInfo("main: exec returned");
	tv.wait();
	qInfo("main: threads joined");

	// Explicit cleanup, after every producer has stopped.
	std::ofstream("demo.state") << "jobsDone=" << jobsDone.load() << "\n";
	qInfo("main: cleanup done, jobsDone=%d", jobsDone.load());
	qInfo("main: return 0");
	return 0;
}
