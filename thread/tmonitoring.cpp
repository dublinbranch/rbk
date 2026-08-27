#include "tmonitoring.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/gitTrick/buffer.h"
#include "rbk/minMysql/min_mysql.h"
#include "threadstatush.h"
#ifdef WITH_Jemalloc
#include "rbk/jemalloc/jemutil.h"
#endif
#include <QDateTime>
#include <QElapsedTimer>
#include <atomic>
#include <cctype>
#include <cstring>
#include <filesystem>
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

	std::string info() const {
		auto delta = QDateTime::currentSecsSinceEpoch() - restartedOn;
		auto ps    = (double)request / (double)delta;
		return fmt::format(R"(
<td>{}</td>
<td>{:.2f} / s</td>
<td>{:.2f}</td>
<td>{:.2f}</td>
<td>{:.2f}</td>
<td>{}</td>
<td>{}</td>
<td>{:.2f}</td>
)",
		                   request.load(),
		                   ps,
		                   ((double)timing.flush) / 1E9,
		                   ((double)timing.sqlFetch) / 1E9,
		                   ((double)timing.sqlServer) / 1E9,
		                   timing.sqlDone.load(),
		                   timing.sqlReconnect.load(),
		                   ((double)timing.total) / 1E9);
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

int countOpenFdsStatus() {
	std::error_code ec;
	int             n = 0;
	for (const auto& e : std::filesystem::directory_iterator("/proc/self/fd", ec)) {
		(void)e;
		++n;
	}
	return ec ? -1 : n;
}

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

std::string hostMemLine() {
	const auto avail = meminfoKb("MemAvailable");
	const auto total = meminfoKb("MemTotal");
	const auto swapF = meminfoKb("SwapFree");
	const auto swapT = meminfoKb("SwapTotal");
	const auto warn  = (avail / 1024) < 256 ? "  [warn < 256 MB]" : "";
	return fmt::format("Host mem    : {:.0f} / {:.0f} MB available{}  swap {:.0f} / {:.0f} MB free\n",
	                   (double)avail / 1024.0, (double)total / 1024.0, warn,
	                   (double)swapF / 1024.0, (double)swapT / 1024.0);
}

std::string identityLine() {
	char host[256]{};
	if (gethostname(host, sizeof(host) - 1) != 0) {
		std::strncpy(host, "?", sizeof(host) - 1);
	}
	const auto uid = ::geteuid();
	return fmt::format("Host        : {}  euid {}{}\n",
	                   host, uid, uid == 0 ? " (root)" : "");
}

std::string mysqlLine() {
	if (!mainDB || !mainDB->hasConf()) {
		return "MySQL       : not configured\n";
	}
	std::string ping = "unknown";
	QElapsedTimer t;
	t.start();
	try {
		auto conn = mainDB->getConn();
		if (!conn) {
			ping = "no connection";
		} else if (mysql_ping(conn) != 0) {
			ping = fmt::format("ping fail {} ({} ms)", mysql_error(conn), t.elapsed());
		} else {
			ping = fmt::format("ok ({} ms)", t.elapsed());
		}
	} catch (const std::exception& e) {
		ping = fmt::format("ping exception: {}", oneLine(QString::fromUtf8(e.what()), 80));
	}
	const auto err = DB::lastErrorText();
	const auto errLine = err.isEmpty()
	                         ? std::string("none")
	                         : fmt::format("[{}] {} ({})", DB::lastErrorCodeSnapshot(),
	                                       oneLine(err), ageAgo(DB::lastErrorAtMs()));
	return fmt::format("MySQL       : {}\nMySQL last  : reconnect {}  error {}\n",
	                   ping, ageAgo(DB::lastReconnectAtMs()), errLine);
}

} // namespace

string composeStatus() {
	auto rqs = ((double)request / (double)(QDateTime::currentMSecsSinceEpoch() - startedAt)) * 1000.0;
	//TODO convert to json in master so can be used by hacheck easily

	string generalStatus;
	generalStatus.reserve(64000);

	generalStatus += R"(<!DOCTYPE html><html role='document'><head><meta charset='utf-8'><meta content='en' name='language'>
<style>
 .tableG1 {
	font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
	border-collapse: collapse;
	width: 100%;
}

 .tableG1 td, .chameleon-table th  {
	border: 1px solid #ddd;
	padding: 8px;
}

 .tableG1 td{
	text-align:right;
}

 .tableG1 tr:nth-child(even){background-color: #f2f2f2;}

 .tableG1 tr:hover {background-color: #ddd;}

 .tableG1 th , .chameleon-table th {
	padding-top: 12px;
	padding-bottom: 12px;z
	text-align: left;
	background-color: #4CAF50;
	color: white;
}

</style>
</head>
<body>
)";

	struct rlimit nofile {};
	getrlimit(RLIMIT_NOFILE, &nofile);
	struct rusage ru {};
	getrusage(RUSAGE_SELF, &ru);
	const auto upSec = (QDateTime::currentMSecsSinceEpoch() - startedAt) / 1000;

	std::string jemallocLine;
#ifdef WITH_Jemalloc
	JEMUtil::refreshStatsCache();
	jemallocLine = fmt::format("Jemalloc    : {:.1f} MB allocated, {:.1f} MB resident\n",
	                           (double)JEMUtil::readU64("stats.allocated") / 1048576.0,
	                           (double)JEMUtil::readU64("stats.resident") / 1048576.0);
#endif

	generalStatus += fmt::format(R"(
<pre>
Request done: {}
Request /s  : {:.1f}
Free Thread : {}
Used Thread : {}
Exception   : {}
Stacker cache: {}
Open fds    : {} / {} (hard {})
PID         : {}
Uptime      : {}
RSS max     : {:.1f} MB
{}{}{}{}Git revision: {}
Compiled at : {} UTC
</pre>
)", // conf().workerLimit - threadFree
	                             request.load(),
	                             rqs,
	                             threadStatus.free.load(),
	                             getThreadCount(),
	                             exceptionThrown.load(),
	                             stackerResolveCacheInfo(),
	                             countOpenFdsStatus(),
	                             nofile.rlim_cur,
	                             nofile.rlim_max,
	                             ::getpid(),
	                             formatUptime(upSec),
	                             (double)ru.ru_maxrss / 1024.0,
	                             jemallocLine,
	                             hostMemLine(),
	                             identityLine(),
	                             mysqlLine(),
	                             GIT_STATUS_buffer,
	                             COMPILATION_TIME_buffer);

	generalStatus += R"(
<hr>
All time are in seconds
<table class="tableG1">
<tr>
	<th>Slot</th>
	<th>Request</th>
	<th>RPS</th>
	<th>Flush time</th>
	<th>Sql Fetch</th>
	<th>Sql Server</th>
	<th>Sql Done</th>
	<th>Mysql Reconnection</th>
	<th>Total time</th>
</tr>)";

	generalStatus += "<tr><td>1m</td>" + m1.info() + "<tr>";
	generalStatus += "<tr><td>5m</td>" + m5.info() + "<tr>";
	generalStatus += "<tr><td>30m</td>" + m30.info() + "<tr>";
	generalStatus += "<tr><td>300m</td>" + m300.info() + "<tr>";

	string sql;
	generalStatus += R"(
</table>
<hr>
All time are in ms 
<table class="tableG1">
<tr>
	<th>pid</th>
	<th>state</th>
	<th>total</th>
	<th>flush</th>
	<th>execution</th>
	<th>IO</th>
	<th>sqlImmediate</th>
	<th>sqlDeferred</th>
	<th>curlImmediate</th>
	<th>curlDeferred</th>
	<th>ClickHouse</th>
</tr>)";

	for (auto& [x, t] : threadStatus.pool) {
		auto& m = t->time;
		if (t->state == ThreadState::MyQuery) {
			sql += fmt::format("{} : {} \n <br>", t->tid, t->sql);
		}
		generalStatus += fmt::format(R"(
<tr><td>{}</td> <td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td>
)",
		                             t->tid,
		                             asString(t->state),
		                             static_cast<double>(m.total()) / 1E6,
		                             static_cast<double>(m.flush) / 1E6,
		                             static_cast<double>(m.execution()) / 1E6,
		                             static_cast<double>(m.IO.nsecsElapsed()) / 1E6,
		                             static_cast<double>(m.sqlImmediate) / 1E6,
		                             static_cast<double>(m.sqlDeferred) / 1E6,
		                             static_cast<double>(m.curlImmediate) / 1E6,
		                             static_cast<double>(m.curlDeferred) / 1E6,
		                             static_cast<double>(m.clickHouse.nsecsElapsed()) / 1E6);
	}
	generalStatus += "</table>";
	generalStatus += "<hr>" + sql;
	generalStatus += "\n<hr>";
	return generalStatus;
}
