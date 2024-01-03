#include "httpUser.h"
#include "config.h"
#include "rbk/caching/apcu2.h"
#include "rbk/hash/salt.h"
#include "rbk/hash/sha.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlcomposer.h"
#include "rbk/misc/b64.h"

extern DB* mainDB;
using namespace std;
u64 HttpUser::getId() const {
	return id;
}

HttpUser::ErrorCode HttpUser::getErrorCode() const {
	return errorCode;
}

std::string_view HttpUser::getErrorString() const {
	return asSWString(getErrorCode());
}

void HttpUser::promoteToAdmin() {
	SqlComposer c(mainDB);
	c.setTable("users");
	c.push("level", "admin");
	c.where->push("email", email);
	mainDB->query(c.composeUpdate());
}

void HttpUser::fromRow(const sqlRow& row) {
	admin = row["level"] == "admin";
	row.rq("id", id);
	row.rq("enabled", enabled);
	row.rq("email", email);
}

HttpUser::ErrorCode HttpUser::fromEmail(const QString& _email) {
	SqlComposer c(mainDB);
	c.setTable("users");
	c.where->push("email", _email);
	auto row = mainDB->queryCacheLine2(c.composeSelectAll(), 0, false);
	if (row.empty()) {
		return ErrorCode::invalidEmail;
	}
	fromRow(row);
	return ErrorCode::ok;
}

QByteArray HttpUser::crypt2(const QString& email, const QString& password) {
	auto sale = salt(string(), 17);
	return crypt1(email, password, sale);
}

bool HttpUser::verifyPassword(const QString& _email, const QString& password, const QByteArray& hashed) const {
	auto salt = hashed.mid(0, 17).toStdString();
	auto c2   = crypt1(_email, password, salt);
	return c2 == hashed;
}

QByteArray HttpUser::crypt1(const QString& email, const QString& password, const std::string& salt) {
	auto base  = F8("{}{}{}{}", salt, email, password, conf().pp);
	auto sha   = sha256(base, true);
	auto final = F8("{}{}", salt, sha);
	return final;
}

bool HttpUser::isLogged() const {
	if (errorCode != ErrorCode::ok) {
		return false;
	}
	return logged;
}

bool HttpUser::isAdmin() const {
	return admin;
}

std::shared_ptr<HttpUser> HttpUser::fromSession(const std::string& sessionId) {
	auto user = apcuFetch<HttpUser>(sessionId);
	return user;
}

HttpUser::ErrorCode HttpUser::login(const QString& _email, const QString& password) {
	SqlComposer c(mainDB);
	c.setTable("users");
	c.where->push("email", _email);
	auto row = mainDB->queryCacheLine2(c.composeSelectAll(), 0, false);
	if (row.empty()) {
		errorCode = ErrorCode::invalidEmail;
		return errorCode;
	}

	auto hashed = row["hashed"];
	if (!verifyPassword(_email, password, hashed)) {
		errorCode = ErrorCode::invalidPassword;
		return errorCode;
	}

	errorCode = ErrorCode::ok;
	logged    = true;
	fromRow(row);
	return errorCode;
}

string HttpUser::registerSession() {
	//we will just concat the microtime at the end of the salt to be sure is unique...
	sessionId = F("{}{}", salt(string(), 32), QDateTime::currentMSecsSinceEpoch());
	apcuStore(sessionId, *this, conf().userSessionTTL);
	return sessionId;
}

void HttpUser::logout() {
	//until this thread is over, we still have something around so better flat out the session
	id     = 0;
	logged = 0;
	apcuRemove(sessionId);
}

void HttpUser::deleteUser() {
	auto sql = F("DELETE FROM user_has_container WHERE userId = {}", id);
	mainDB->query(sql);

	sql = F("DELETE FROM users WHERE id = {}", id);
	mainDB->query(sql);
}

HttpUser HttpUser::createUser(const QString& email, const QString& password) {

	HttpUser u;
	{
		//check if the user already exists
		SqlComposer c(mainDB);
		c.setTable("users");
		c.where->push("email", email);
		auto res = mainDB->queryCacheLine2(c.composeSelect("id"), 0, false);
		if (!res.empty()) {
			u.errorCode = ErrorCode::emailAlreadyExists;
			return u;
		}
	}

	u.enabled = true;

	SqlComposer c(mainDB);
	c.setTable("users");
	c.push("email", email);
	c.push({{"hashed"}, SScol::Value{base64this(crypt2(email, password)).toStdString(), true}});
	c.push("enabled", u.enabled);
	c.push("level", "user");
	mainDB->query(c.composeInsert());
	u.id        = mainDB->lastId();
	u.errorCode = ErrorCode::ok;
	u.email     = email;
	return u;
}

std::string HttpUser::getSessionUA() const {
	return sessionUA;
}

std::string HttpUser::getSessionIP() const {
	return sessionIP;
}

void HttpUser::setErrorCode(ErrorCode newErrorCode) {
	errorCode = newErrorCode;
}

// public
// static function fromSession()
//     : User {
// 	$oldIp = $_SESSION["ip"];
// 	$user  = new User();
// 	if ($oldIp != $_SERVER["REMOTE_ADDR"]) {
// 		$user->errorCode = 1;
// 		if (!conf()->debug->testMode) {
// 			session_destroy();
// 			header('Location: /login.php?reason='.$user->errorCode);
// 		}
// 	} else {
// 		$user->id = (int)$_SESSION["userId"];
// 		$user->fromId();
// 		$user->logged = true;
// 	}
// 	return $user;
// }

// public
// function fromId(int $id)
//     : int {
// 	$sql = "SELECT * FROM users WHERE id = $id";
// 	$row = db()->getLine($sql);
// 	if (!sizeof($row)) {
// 		$this->errorCode = 2;
// 	} else {
// 		$this->fromRow($row);
// 	}
// 	return $this->id;
// }
