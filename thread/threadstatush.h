#ifndef THREADSTATUSH_H
#define THREADSTATUSH_H

#include "rbk/mapExtensor/hmap.h"
#include "rbk/misc/intTypes.h"

#include <QString>
#include <memory>
#include <thread>

#include <QElapsedTimer>

enum class ThreadState {
	Idle,
	Beast,
	Immediate,
	Deferred,
	MyQuery,
	MyCache,
	ClickHouse,
	cURL,
};

class ElapsedTimerV2 {
      public:
	void start();
	i64  pause();

	i64 nsecsElapsed() const;

      private:
	bool          paused = true;
	QElapsedTimer timer;
	i64           total = 0;
};

class ThreadStatus {
      public:
	struct Timing {
		ElapsedTimerV2 timer;

		//Total execution time including log writing, clickhouse, sql, whatever until NOW (or is called pause)
		i64            total() const;
		ElapsedTimerV2 clickHouse;

		//Time spent doing IO, mostly reading the disk cache
		ElapsedTimerV2 IO;
		//Once data is fully sent to browser
		i64 flush = 0;
		//The actual time spend executing code
		i64 execution() const;

		//time spent in sql, this is computed only until the delivery, all sql after the http is sent are irrelevant
		i64 sqlImmediate = 0;
		i64 sqlDeferred  = 0;

		i64 curlImmediate = 0;
		i64 curlDeferred  = 0;

		void reset();
		//We need a function as we do the separation between Immediate and Deferred
		void addSqlTime(qint64 addMe);
		void addCurlTime(qint64 addMe);
	};

	struct Status {
		ThreadState state;
		int         tid = 0;
		Timing      time;
		QString     info;
		QByteArray  sql;
	};

	static std::shared_ptr<Status> newStatus();

	hmap<std::thread::id, std::shared_ptr<Status>> pool;
	std::atomic<size_t>                               free{0};
};

#endif // THREADSTATUSH_H
