#ifndef RUNNABLE_H
#define RUNNABLE_H

#include "rbk/misc/intTypes.h"
#include "rbk/string/stringoso.h"
#include <QString>
class DB;

class Runnable {
      public:
	// for debugging
	// true -> runnable() returns always true ("runnable" table not used)
	bool forceRunnable = false;

	// Non copyable
	Runnable& operator=(const Runnable&) = delete;
	Runnable(const Runnable&)            = delete;
	Runnable()                           = default;
	Runnable(DB* db_);
	void setConf(DB* db);
	/**
	 * @brief coolDown will check the last time a "key" has been activated
	 * @param key
	 * @param time
	 * @return true: we can run, false: do not run
	 */
	[[nodiscard]] bool runnable(const QStringAdt& key, i64 second, double multiplier = 1);
	//To make it fully compatible with swaptronic old one
	[[nodiscard]] bool operator()(const QStringAdt& key, i64 second, double multiplier = 1);
	[[nodiscard]] bool runnable_64(const QString& key, i64 second, double multiplier = 1);
	/**
	 * @brief runDecay
	 * @param key
	 * @param second
	 * @param multiplier each time how much longer before a report
	 * @return
	 */

      private:
	DB* db = nullptr;
};

#endif // RUNNABLE_H
