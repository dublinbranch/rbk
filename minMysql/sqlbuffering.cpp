#include "sqlbuffering.h"
#include "min_mysql.h"
#include "rbk/QStacker/qstacker.h"

SQLBuffering::SQLBuffering(DB* _conn, uint _bufferSize, bool _useTRX) {
	if (!_conn) {
		throw ExceptionV2("in nullptr we DO NOT TRUST! fix the code");
	}
	this->conn       = _conn;
	this->bufferSize = _bufferSize;
	this->useTRX     = _useTRX;
}

/**
 * @brief SQLBuffering::~SQLBuffering
 * Exception should not leave destructor, if you want more control call flush manually
 */
SQLBuffering::~SQLBuffering() {
	try {
		flush();
	} catch (std::exception& e) {
		qCritical().noquote() << e.what();
	} catch (...) {
		qCritical() << "unknow exception in " << QStacker16();
	}
}

void SQLBuffering::append(const std::string& sql) {
	if (sql.empty()) {
		return;
	}
	append(QString::fromStdString(sql));
}

void SQLBuffering::append(const QString& sql) {
	if (sql.isEmpty()) {
		return;
	}

	// 0 disable flushing, 1 disable buffering
	if (bufferSize && (uint)buffer.size() >= bufferSize) {
		flush();
	}
	buffer.append(sql);
}

void SQLBuffering::append(const QStringList& sqlList) {
	for (auto& row : sqlList) {
		append(row);
	}
}

void SQLBuffering::flush() {
	if (buffer.isEmpty()) {
		return;
	}
	if (conn == nullptr) {
		throw DBException("you forget to set a usable DB Conn!", DBException::Error::Connection);
	}
	/**
	 * To avoid having a very big packet we split
	 * usually max_allowed_packet is https://mariadb.com/kb/en/server-system-variables/#max_allowed_packet
	   16777216 (16M) >= MariaDB 10.2.4
	        4194304 (4M) >= MariaDB 10.1.7
	        1048576 (1MB) <= MariaDB 10.1.6
	 1073741824 (1GB) (client-side)
	 It should be nice to read the value from the conn ... ?
	 TODO add
	 show variables like "max_allowed_packet"
	 */

	// This MUST be out of the buffered block!  BUT WHY ?
	if (useTRX) {
		conn->query(QBL("START TRANSACTION;"));
	}
	QString currentQuery;
	// TODO just compose the query in utf8, and append in utf8
	for (auto&& line : buffer) {
		currentQuery.append(line);
		currentQuery.append(QSL("\n"));
		// this is UTF16, but MySQL run in UTF8, so can be lower or bigger (rare vey rare but possible)
		// small safety margin + increase size for UTF16 -> UTF8 conversion
		if (((double)currentQuery.size() * 1.3) > maxPacket * 0.75) {
			if (skipWarning) {
				conn->skipWarning = true;
			}
			auto res = conn->queryDeadlockRepeater(currentQuery.toUtf8());
			currentQuery.clear();
		}
	}
	buffer.clear();
	if (!currentQuery.isEmpty()) {
		if (skipWarning) {
			conn->skipWarning = true;
		}
		auto res = conn->queryDeadlockRepeater(currentQuery.toUtf8());
	}
	// This MUST be out of the buffered block! BUT WHY ?
	if (useTRX) {
		conn->query(QBL("COMMIT;"));
	}
}

void SQLBuffering::setUseTRX(bool _useTRX) {
	this->useTRX = _useTRX;
}

void SQLBuffering::clear() {
	buffer.clear();
}
