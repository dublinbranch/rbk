#include "tmonitoring.h"
#include "rbk/HTTP/router.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/gitTrick/buffer.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/misc/escapeH.h"
#include "statusProbes.h"
#include "threadstatush.h"
#ifdef WITH_Jemalloc
#include "rbk/jemalloc/jemutil.h"
#endif
#include <QDateTime>
#include <QElapsedTimer>
#include <atomic>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/resource.h>
#include <unistd.h>

using namespace std;

extern ThreadStatus                       threadStatus;
extern thread_local ThreadStatus::Status* localThreadStatus;

extern DB* mainDB;

struct ATiming {
	atomic<i64> total = 0;
	atomic<i64> flush = 0;
	//Almost all our query are buffered and small so fetch is basically istant
	atomic<i64> sqlFetch = 0;
	//time spent doing the sql (network latency + execution)
	atomic<i64> sqlServer = 0;
	//how many sql we did (all servicing thread + cache)
	atomic<i64> sqlDone = 0;
	// (all servicing thread + cache)
	atomic<i64> sqlReconnect = 0;

	void syncFromDk_S7Db() {
		flush        = localThreadStatus->time.flush;
		auto& st     = mainDB->state.get();
		sqlFetch     = st.totFetchTime;
		sqlServer    = st.totServerTime;
		sqlDone      = st.queryExecuted;
		sqlReconnect = st.reconnection;

		total = localThreadStatus->time.total();
	}
	void clear() {
		total        = 0;
		flush        = 0;
		sqlFetch     = 0;
		sqlServer    = 0;
		sqlDone      = 0;
		sqlReconnect = 0;
	}
};

struct Averager {
	Averager(uint bs) {
		blockSize = bs;
	}
	i64 blockSize = 0;

	i64         restartedOn = 0;
	i64         resetAfter  = 0;
	atomic<i64> request{0};
	ATiming     timing;

	void clear() {
		request = 0;
		timing.clear();
	}

	void bump() {
		auto now = QDateTime::currentSecsSinceEpoch();
		if (now > resetAfter) {
			restartedOn = now;
			resetAfter  = ((now / blockSize) + 1) * blockSize;
			clear();
		}
		request++;
	}

	// <td> cells of one row of the request table: request, rps, then seconds
	std::string info() const {
		using namespace statusProbes;
		const auto delta = QDateTime::currentSecsSinceEpoch() - restartedOn;
		const auto ps    = delta > 0 ? (double)request / (double)delta : 0.0;
		auto       sec   = [](i64 ns) { return td(nFix((double)ns / 1E9, 2)); };
		return td(nInt((u64)request.load())) + td(nFix(ps, 2)) + sec(timing.flush) + sec(timing.sqlFetch) +
		       sec(timing.sqlServer) + td(nInt((u64)timing.sqlDone.load())) + td(nInt((u64)timing.sqlReconnect.load())) +
		       sec(timing.total);
	}
};

size_t getThreadCount() {
	return threadStatus.pool.size();
}

static atomic<uint> request{0};
static auto         startedAt = QDateTime::currentMSecsSinceEpoch();
static Averager     m1(60);
static Averager     m5(300);
static Averager     m30(60 * 30);
static Averager     m300(60 * 300);
// Written once by the config load before the HttpHandler threads exist, read only after
static StatusPageConf statusConf;

void setStatusPageConf(const StatusPageConf& c) {
	statusConf = c;
}

const StatusPageConf& statusPageConf() {
	return statusConf;
}

void requestBeging() {
	localThreadStatus->state = ThreadState::Beast;
	localThreadStatus->time.reset();
	threadStatus.free--;
	//static size_t minFree = 5; //floor(conf().workerLimit * 0.1);
	//if we care about usage, than we have a reasonable num of thread (at least 10)
	// if (minFree && threadStatus.free < minFree) {
	// 	//TODO write on disk about low thread
	// 	for (auto& [x, t] : threadStatus.pool) {
	// 		(void)x;
	// 		(void)t;
	// 		//probably the actual status page is fine, just do a non html version with manual tabling suitable for log ?
	// 	}
	// 	//send a slack warning of all thread used ?
	// 	//but more important, what are the conseguences of using say 100 thread or 200 ? slower / overhead / X ?
	// }
	request++;
	m1.bump();
	m5.bump();
	m30.bump();
	m300.bump();
	//dk.reset();
	//	s7DB.state.get().totFetchTime  = 0;
	//	s7DB.state.get().totServerTime = 0;
	//	s7DB.state.get().reconnection  = 0;
	//	s7DB.state.get().queryExecuted = 0;
}

void requestEnd() {
	localThreadStatus->state = ThreadState::Idle;
	if (localThreadStatus->time.timer.timer.isValid()) {
		localThreadStatus->time.timer.pause();
	}

	m1.timing.syncFromDk_S7Db();
	m5.timing.syncFromDk_S7Db();
	m30.timing.syncFromDk_S7Db();
	m300.timing.syncFromDk_S7Db();
	threadStatus.free++;
}
i64 registerFlushTime() {
	if (localThreadStatus->time.timer.timer.isValid()) {
		localThreadStatus->time.flush = localThreadStatus->time.timer.nsecsElapsed();
	}
	return localThreadStatus->time.flush;
}

namespace {

std::string formatUptime(i64 upSec) {
	const auto h = upSec / 3600;
	const auto m = (upSec % 3600) / 60;
	const auto s = upSec % 60;
	return fmt::format("{}h {:02}m {:02}s", h, m, s);
}

std::string ageAgo(qint64 atMs) {
	if (atMs <= 0) {
		return "never";
	}
	const auto sec = (QDateTime::currentMSecsSinceEpoch() - atMs) / 1000;
	if (sec < 1) {
		return "just now";
	}
	return formatUptime(sec) + " ago";
}

unsigned long meminfoKb(const char* key) {
	std::ifstream in("/proc/meminfo");
	std::string   line;
	const auto    prefix = std::string(key);
	while (std::getline(in, line)) {
		if (line.compare(0, prefix.size(), prefix) != 0) {
			continue;
		}
		std::istringstream iss(line);
		std::string        name;
		unsigned long      kb = 0;
		iss >> name >> kb;
		return kb;
	}
	return 0;
}

std::string oneLine(QString s, int maxLen = 140) {
	s.replace(QLatin1Char('\n'), QLatin1Char(' '));
	s = s.simplified();
	if (s.size() > maxLen) {
		s = s.left(maxLen) + QLatin1String("…");
	}
	return s.toStdString();
}

std::string hostRows() {
	using namespace statusProbes;
	char host[256]{};
	if (gethostname(host, sizeof(host) - 1) != 0) {
		std::strncpy(host, "?", sizeof(host) - 1);
	}
	const auto uid   = ::geteuid();
	const auto avail = meminfoKb("MemAvailable");
	const auto total = meminfoKb("MemTotal");
	const auto swapF = meminfoKb("SwapFree");
	const auto swapT = meminfoKb("SwapTotal");
	std::string r;
	r += rowT("Hostname", fmt::format("{} · euid {}{}", escapeH(host), uid, uid == 0 ? " (root)" : ""));
	r += rowN("Memory available", nBytes((u64)avail * 1024),
	          fmt::format("of {:.0f} MB{}", (double)total / 1024.0, (avail / 1024) < 256 ? " " + warn("under 256 MB") : ""));
	r += rowN("Swap free", nBytes((u64)swapF * 1024), fmt::format("of {:.0f} MB", (double)swapT / 1024.0));
	return r;
}

std::string mysqlRows(bool doPing) {
	using namespace statusProbes;
	if (!mainDB || !mainDB->hasConf()) {
		return rowT("MySQL", "not configured");
	}
	std::string ping = "disabled (statusPage.mysqlPing)";
	if (doPing) {
		QElapsedTimer t;
		t.start();
		try {
			auto conn = mainDB->getConn();
			if (!conn) {
				ping = warn("no connection");
			} else if (mysql_ping(conn) != 0) {
				ping = warn(fmt::format("fail {} ({} ms)", escapeH(mysql_error(conn)), t.elapsed()));
			} else {
				ping = fmt::format("ok ({} ms)", t.elapsed());
			}
		} catch (const std::exception& e) {
			ping = warn(fmt::format("exception: {}", escapeH(oneLine(QString::fromUtf8(e.what()), 80))));
		}
	}
	const auto err     = DB::lastErrorText();
	const auto errLine = err.isEmpty()
	                         ? std::string("none")
	                         : fmt::format("[{}] {} ({})", DB::lastErrorCodeSnapshot(),
	                                       escapeH(oneLine(err)), ageAgo(DB::lastErrorAtMs()));
	return rowT("MySQL ping", ping) + rowT("MySQL reconnect", ageAgo(DB::lastReconnectAtMs())) + rowT("MySQL last error", errLine);
}


} // namespace

string composeStatus() {
	using namespace statusProbes;
	auto rqs = ((double)request / (double)(QDateTime::currentMSecsSinceEpoch() - startedAt)) * 1000.0;
	//TODO convert to json in master so can be used by hacheck easily
	const auto& sc = statusConf;

	string out;
	out.reserve(64000);
	out += statusProbes::pageHead;

	struct rlimit nofile {};
	getrlimit(RLIMIT_NOFILE, &nofile);
	struct rusage ru {};
	getrusage(RUSAGE_SELF, &ru);
	const auto upSec = (QDateTime::currentMSecsSinceEpoch() - startedAt) / 1000;

	const bool process = sc.process.value_or(true);
	const bool host    = sc.host.value_or(true);
	const bool network = sc.network.value_or(true);
	const bool unitNet = sc.unitNet.value_or(true);
	const bool sockets = sc.sockets.value_or(true);
	const bool threads = sc.threads.value_or(true);

	char hostname[256]{};
	gethostname(hostname, sizeof(hostname) - 1);
	out += fmt::format("<h1>Status · {}</h1>\n<nav><a href=\"#service\">Service</a>", escapeH(hostname));
	if (process) {
		out += "<a href=\"#process\">Process</a>";
	}
	out += "<a href=\"#host\">Host</a>";
	if (network || unitNet) {
		out += "<a href=\"#network\">Network</a>";
	}
	out += "<a href=\"#requests\">Requests</a><a href=\"#threads\">Threads</a><a href=\"#cache\">Cache</a>";
	if (sockets) {
		out += fmt::format("<a href=\"{}\">TCP sockets</a>", statusSocketsPath);
	}
	out += "</nav>\n";

	// One fstat() walk for both the count and the kinds: at 10K sockets each walk is ~6 ms of this thread
	const auto fds     = fdCount(sc.fdKinds.value_or(true));
	const auto fdShare = nofile.rlim_cur ? 100.0 * (double)fds.open / (double)nofile.rlim_cur : 0.0;

	string svc;
	svc += rowN("Requests", kInt((u64)request.load()));
	svc += rowN("Requests / s", kFix(rqs, ""), "average since start");
	svc += rowN("Worker threads", kInt(getThreadCount()));
	svc += rowN("Workers free", kInt(threadStatus.free.load()));
	svc += rowN("Exceptions", kInt((u64)exceptionThrown.load()));
	svc += rowN("Open fds", kInt(fds.open),
	            fmt::format("soft limit {} (hard {}){}", nofile.rlim_cur, nofile.rlim_max, fdShare > 80 ? " " + warn("over 80% of the soft limit") : ""));
	if (!fds.kinds.empty()) {
		svc += rowT("Fd kinds", fds.kinds);
	}
	svc += rowN("RSS max", nBytes((u64)ru.ru_maxrss * 1024));
#ifdef WITH_Jemalloc
	JEMUtil::refreshStatsCache();
	svc += rowN("Jemalloc allocated", nBytes(JEMUtil::readU64("stats.allocated")));
	svc += rowN("Jemalloc resident", nBytes(JEMUtil::readU64("stats.resident")));
#endif
	svc += rowT("Stacker cache", escapeH(stackerResolveCacheInfo()));
	svc += rowT("PID", fmt::format("{}", ::getpid()));
	svc += rowT("Uptime", formatUptime(upSec));
	svc += mysqlRows(sc.mysqlPing.value_or(true));
	svc += rowT("Git revision", escapeH(GIT_STATUS_buffer.toStdString()));
	svc += rowT("Compiled at", fmt::format("{} UTC", COMPILATION_TIME_buffer));

	out += "<div class=\"grid\">\n" + kvSection("service", "Service", svc);
	if (process) {
		out += kvSection("process", "Process", processRows(static_cast<double>(upSec), getThreadCount()));
	}
	out += "</div>\n";

	if (host) {
		out += hostSection(sc.diskPaths.value_or(std::vector<std::string>{}), hostRows());
	} else {
		out += kvSection("host", "Host", hostRows());
	}
	if (network || unitNet) {
		out += networkSection(network, unitNet, sockets ? statusSocketsPath : "");
	}

	out += R"(<section id="requests"><h2>Requests</h2>
<p class="note">Per time slot, times in seconds</p>
<table class="t">
<tr class="hd"><th>slot</th><th>requests</th><th>rps</th><th>flush s</th><th>sql fetch s</th><th>sql server s</th><th>sql done</th><th>mysql reconnects</th><th>total s</th></tr>
)";
	out += "<tr><th>1m</th>" + m1.info() + "</tr>\n";
	out += "<tr><th>5m</th>" + m5.info() + "</tr>\n";
	out += "<tr><th>30m</th>" + m30.info() + "</tr>\n";
	out += "<tr><th>300m</th>" + m300.info() + "</tr>\n";
	out += "</table></section>\n";

	out += R"(<section id="threads"><h2>Worker threads</h2>
<p class="note">Times of the current or last request in ms)";
	if (threads) {
		out += ". kstate / core / cpu / rq wait come from the kernel, cpu and rq wait in s since the thread started";
	}
	out += R"(</p>
<table class="t">
<tr class="hd"><th>tid</th><th>state</th><th>total</th><th>flush</th><th>execution</th><th>IO</th><th>sql immediate</th><th>sql deferred</th><th>curl immediate</th><th>curl deferred</th><th>ClickHouse</th>)";
	if (threads) {
		out += "<th>kstate</th><th>core</th><th>cpu s</th><th>rq wait s</th>";
	}
	out += "</tr>\n";

	string           sql;
	std::vector<int> poolTids;
	auto             ms = [](double ns) { return td(nFix(ns / 1E6, 2)); };
	for (auto& [x, t] : threadStatus.pool) {
		auto& m = t->time;
		if (t->state == ThreadState::MyQuery) {
			sql += fmt::format("{} : {}\n", t->tid, escapeH(t->sql));
		}
		poolTids.push_back(t->tid);
		out += fmt::format("<tr>{}<td>{}</td>{}{}{}{}{}{}{}{}{}{}</tr>\n",
		                   td(fmt::format("{}", t->tid)),
		                   asString(t->state).toStdString(),
		                   ms(static_cast<double>(m.total())),
		                   ms(static_cast<double>(m.flush)),
		                   ms(static_cast<double>(m.execution())),
		                   ms(static_cast<double>(m.IO.nsecsElapsed())),
		                   ms(static_cast<double>(m.sqlImmediate)),
		                   ms(static_cast<double>(m.sqlDeferred)),
		                   ms(static_cast<double>(m.curlImmediate)),
		                   ms(static_cast<double>(m.curlDeferred)),
		                   ms(static_cast<double>(m.clickHouse.nsecsElapsed())),
		                   threads ? kernelCells(threadKernel(t->tid)) : string());
	}
	out += "</table></section>\n";
	if (threads) {
		out += otherThreadsTable(poolTids);
	}
	if (!sql.empty()) {
		out += "<section><h2>Running SQL</h2>\n<pre>" + sql + "</pre></section>\n";
	}
	return out;
}

string composeSocketsStatus() {
	string page = statusProbes::pageHead;
	page += fmt::format("<h1>TCP sockets</h1>\n<nav><a href=\"../{}\">Back to the status page</a></nav>\n", statusPagePath);
	page += statusProbes::socketsBody(statusConf.socketTop.value_or(10));
	page += "</body></html>\n";
	return page;
}
