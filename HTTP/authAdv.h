#ifndef RBK_HTTP_AUTHADV_H
#define RBK_HTTP_AUTHADV_H

#include "rbk/HTTP/authHandler.h"
#include "rbk/misc/intTypes.h"

#include <boost/json/array.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

class DB;

/**
 * Groups and read / write bits, on top of rbk::Auth (which does not change).
 *
 * A Priv is one area of the product, one line in an enum of the project. Each Priv has a read and
 * a write bit. A group is a row with a name and, for each Priv, the two ticks. A login is in one
 * group. Root passes every check, so nobody can lock the system out by removing a tick. A Priv
 * with no tick is closed for everybody but root.
 *
 * The tables, for the next project:
 *
 *   CREATE TABLE aclGroup (
 *     id int unsigned NOT NULL AUTO_INCREMENT, name varchar(64) NOT NULL,
 *     note varchar(255) NOT NULL DEFAULT '', PRIMARY KEY (id), UNIQUE KEY name (name));
 *   CREATE TABLE aclGroupPriv (
 *     groupId int unsigned NOT NULL, priv varchar(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 *     canRead tinyint unsigned NOT NULL DEFAULT 0, canWrite tinyint unsigned NOT NULL DEFAULT 0,
 *     PRIMARY KEY (groupId, priv));
 *
 * Wiring: fill rbk::AuthAdv::conf() at startup, next to rbk::Auth::conf(), then
 *   {"api/xyz", rbk::AuthAdv::canRead<MyPriv::X, myHandler>}
 * The check is in the routing table on purpose: a test can read the table, and a check somebody
 * forgot inside a handler is invisible. A route that reads on GET and writes on POST takes canRead
 * and calls needWrite on its POST branch.
 *
 * Which group a login is in is up to the project: rbk never touches the user class.
 */
namespace rbk::AuthAdv {

enum class Op : u8 {
	read  = 1,
	write = 2
};

struct PrivDef {
	int              id;    // the index in Bits, usually the value of the enum
	std::string_view name;  // the key in the DB, so an enum reorder is harmless
	std::string_view label; // the line on the page
	bool             audited = true; // false: its routes were never read for a group; the page warns
};

using Bits = std::vector<u8>; // one entry for each PrivDef id, Op as a mask

struct Conf {
	DB*                                     db = nullptr; // for the handlers of the matrix
	std::vector<PrivDef>                    privs;
	std::function<bool()>                   isRoot;
	std::function<const Bits&()>            bitsOfCurrentUser; // stored by the project
	std::function<void(const std::string&)> audit;
	// the logins placed in a group by hand: an array of objects, shown on the page
	std::function<boost::json::array(u32 group)> membersOf;
	// a group the page must not delete
	std::function<bool(u32 group)> isProtectedGroup;
};

// the singleton filled by the project at startup, before the server starts
Conf& conf();
bool  can(int priv, Op op);
// The ticks of one group. An unknown name in the DB is ignored and logged once per name: this runs
// on each request, and a Priv removed from the enum leaves such a row. Group 0 = no ticks, no query.
Bits loadBits(DB& db, const std::vector<PrivDef>& privs, u32 group);

// The handlers of the matrix. They do not check who calls: route them behind root only, because
// whoever edits the ticks can give themselves everything. JSON answers. Each change writes one
// audit row. An untick writes 0 and keeps the row, so a re-run of a seed cannot bring it back.
void matrixApi(PMFCGI& status, Payload& payload);      // GET
void setBitApi(PMFCGI& status, Payload& payload);      // POST {group, priv, op: "read"|"write", on}
void groupCreateApi(PMFCGI& status, Payload& payload); // POST {name, note?}
void groupCloneApi(PMFCGI& status, Payload& payload);  // POST {from, name}
void groupRenameApi(PMFCGI& status, Payload& payload); // POST {group, name}
void groupDeleteApi(PMFCGI& status, Payload& payload); // POST {group}

template <auto priv, Op op, void (*handler)(PMFCGI& status, Payload& payload)>
void guardOp(PMFCGI& status, Payload& payload) {
	auto& base = rbk::Auth::conf();
	if (!base.isLogged || !base.isLogged()) {
		rbk::Auth::denyRoute(status, payload, 401);
		return;
	}
	if (!can(static_cast<int>(priv), op)) {
		rbk::Auth::denyRoute(status, payload, 403);
		return;
	}
	handler(status, payload);
}

template <auto priv, void (*handler)(PMFCGI& status, Payload& payload)>
void canRead(PMFCGI& status, Payload& payload) {
	guardOp<priv, Op::read, handler>(status, payload);
}

template <auto priv, void (*handler)(PMFCGI& status, Payload& payload)>
void canWrite(PMFCGI& status, Payload& payload) {
	guardOp<priv, Op::write, handler>(status, payload);
}

// the POST branch of a mixed route; false = the 403 and its audit row are done
template <auto priv>
bool needWrite(PMFCGI& status, Payload& payload) {
	if (can(static_cast<int>(priv), Op::write)) {
		return true;
	}
	rbk::Auth::denyRoute(status, payload, 403);
	return false;
}

} // namespace rbk::AuthAdv

#endif // RBK_HTTP_AUTHADV_H
