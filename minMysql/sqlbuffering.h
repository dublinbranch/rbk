#ifndef SQLBUFFERING_H
#define SQLBUFFERING_H

#include <QString>
#include <QStringList>

class DB;

/**
 * @brief The SQLBuffering class it is a set of SQL queries with a flashing system which allows to execute
 * the queries (manually or automatically)
 */
class SQLBuffering {
      public:
	DB*         conn       = nullptr;
	uint        bufferSize = 1000;
	QStringList buffer;

	bool skipWarning = false;

	SQLBuffering() = default;
	/**
	 * @brief SQLBuffering
	 * @param _conn
	 * @param _bufferSize 0 disable auto flushing, 1 disable buffering (if you want to wrap lot of stuff in a TRX use 0)
	 */
	SQLBuffering(DB* _conn, uint _bufferSize = 1000, bool _useTRX = true);
	~SQLBuffering();
	void append(const std::string& sql);
	void append(const QString& sql);
	void append(const QStringList& sqlList);
	void flush();
	void setUseTRX(bool _useTRX);
	void clear();

      private:
	// https://mariadb.com/kb/en/server-system-variables/#max_allowed_packet in our system is always 16M atm
	static const uint maxPacket = 16E6;
	// Set as false in case we are running inside another TRX
	bool useTRX = true;
};

#endif // SQLBUFFERING_H
