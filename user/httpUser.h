#ifndef HOME_ROY_PUBLIC_DITER_CLASS_USER_H
#define HOME_ROY_PUBLIC_DITER_CLASS_USER_H

#include "rbk/misc/intTypes.h"
#include <QString>

class sqlRow;
class HttpUser;

using HttpUserPtr = std::shared_ptr<HttpUser>;

class HttpUser {
      public:
      //keep in sync with userAuditTrail::errorCode in the DB
	enum ErrorCode {
		invalidState        = 0, //default value, means a logic error as we are reading a default initialized object
		ok                  = 1, //everything is ok
		ipMismatch          = 2, //in case someone change ip during the check is enabled (can be disabled in config)
		userIdDoesNotExists = 3, //in case someone read from the DB and the user has been deleted
		invalidEmail        = 4,
		invalidPassword     = 5,
		emailAlreadyExists  = 6,
		invalidSession      = 7, //in case the session is not valid
		privilegeError      = 8, //in case the user is not allowed to do something
	};

	//ErrorCode verifyIP(std::string currentIp);

	[[nodiscard]] bool isLogged() const;
	[[nodiscard]] bool isAdmin() const;

	[[nodiscard]] static HttpUserPtr fromSession(const std::string& sessionId);
	virtual ErrorCode                login(const QString& email, const QString& password);
	std::string                      registerSession();
	void                             logout();

	void deleteUser();

	ErrorCode fromEmail(const QString& email);
	//ErrorCode fromId();
	u64              getId() const;
	ErrorCode        getErrorCode() const;
	std::string_view getErrorString() const;

	void promoteToAdmin();
	
	void setErrorCode(ErrorCode newErrorCode);
	
	std::string getSessionIP() const;
	
	std::string getSessionUA() const;
	
	protected:
	static HttpUser createUser(const QString& email, const QString& password);

      private:
	bool    logged  = false;
	bool    admin   = false;
	u64     id      = 0;
	bool    enabled = false;
	QString email;

	std::string sessionIP;
	std::string sessionUA;
	//double check done by the server but need JS cooperation on the client side (as is stored into the session storage)
	std::string sessionToken;
	//this is the user cookie
	std::string sessionId;

	ErrorCode errorCode = ErrorCode::invalidState;

	void fromRow(const sqlRow& row);

	static QByteArray crypt1(const QString& email, const QString& password, const std::string& salt);
	static QByteArray crypt2(const QString& email, const QString& password);
	bool              verifyPassword(const QString& email, const QString& password, const QByteArray& hashed) const;
};

#endif // HOME_ROY_PUBLIC_DITER_CLASS_USER_H
