#include "tmonitoring.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/minMysql/min_mysql.h"
#include "threadstatush.h"
#include <atomic>
#include <cstdint>

using namespace std;

extern ThreadStatus                       threadStatus;
extern thread_local ThreadStatus::Status* localThreadStatus;

extern DB s7DB;

struct ATiming {
	atomic<uint64_t> total = 0;
	atomic<uint64_t> flush = 0;
	//Almost all our query are buffered and small so fetch is basically istant
	atomic<uint64_t> sqlFetch = 0;
	//time spent doing the sql (network latency + execution)
	atomic<uint64_t> sqlServer = 0;
	//how many sql we did (all servicing thread + cache)
	atomic<uint64_t> sqlDone = 0;
	// (all servicing thread + cache)
	atomic<uint64_t> sqlReconnect = 0;

	void syncFromDk_S7Db() {

		total += localThreadStatus->time.total();
		flush += localThreadStatus->time.flush;
		//		auto& st = s7DB.state.get();
		//		sqlFetch += st.totFetchTime;
		//		sqlServer += st.totServerTime;
		//		sqlDone += st.queryExecuted;
		//		sqlReconnect += st.reconnection;
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
	uint blockSize = 0;

	double       restartedOn = 0;
	double       resetAfter  = 0;
	atomic<uint> request{0};
	ATiming      timing;

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
		double delta = QDateTime::currentSecsSinceEpoch() - restartedOn;
		auto   ps    = (double)request / delta;
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
		                   request,
		                   ps,
		                   ((double)timing.flush) / 1E9,
		                   ((double)timing.sqlFetch) / 1E9,
		                   ((double)timing.sqlServer) / 1E9,
		                   timing.sqlDone,
		                   timing.sqlReconnect,
		                   ((double)timing.total) / 1E9);
	}
};

atomic<int>         threadFree{0};
static atomic<uint> request{0};
static uint64_t     startedAt = QDateTime::currentMSecsSinceEpoch();
static Averager     m1(60);
static Averager     m5(300);
static Averager     m30(60 * 30);
static Averager     m300(60 * 300);

void requestBeging() {
	localThreadStatus->state = ThreadState::Beast;
	localThreadStatus->time.reset();
	threadFree--;
	static int minFree = 5; //floor(conf().workerLimit * 0.1);
	//if we care about usage, than we have a reasonable num of thread (at least 10)
	if (minFree && threadFree < minFree) {
		//TODO write on disk about low thread
		for (auto& [x, t] : threadStatus.pool) {
			(void)x;
			(void)t;
			//probably the actual status page is fine, just do a non html version with manual tabling suitable for log ?
		}
		//send a slack warning of all thread used ?
		//but more important, what are the conseguences of using say 100 thread or 200 ? slower / overhead / X ?
	}
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
	localThreadStatus->time.timer.pause();

	m1.timing.syncFromDk_S7Db();
	m5.timing.syncFromDk_S7Db();
	m30.timing.syncFromDk_S7Db();
	m300.timing.syncFromDk_S7Db();
	threadFree++;
}
void registerFlushTime() {
	localThreadStatus->time.flush = localThreadStatus->time.timer.nsecsElapsed();
}

string composeStatus() {
	auto rqs = ((double)request / (QDateTime::currentMSecsSinceEpoch() - startedAt)) * 1000.0;
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

	generalStatus += fmt::format(R"(
<pre>
Request done: {}
Request /s  : {:.1f}
Free Thread : {}
Used Thread : {}
Exception   : {}
</pre>
)", // conf().workerLimit - threadFree
	                             request, rqs, threadFree, 555, exceptionThrown);

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
		                             m.total() / 1E6,
		                             m.flush / 1E6,
		                             m.execution() / 1E6,
		                             m.IO.nsecsElapsed() / 1E6,
		                             m.sqlImmediate / 1E6,
		                             m.sqlDeferred / 1E6,
		                             m.curlImmediate / 1E6,
		                             m.curlDeferred / 1E6,
		                             m.clickHouse.nsecsElapsed() / 1E6);
	}
	generalStatus += "</table>";
	generalStatus += "<hr>" + sql;
	generalStatus += "\n<hr>";
	return generalStatus;
}
