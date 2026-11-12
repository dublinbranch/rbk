#include "authAdv.h"

#include "rbk/BoostJson/extra.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"

#include <QDebug>
#include <algorithm>
#include <boost/json.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <set>

namespace bj = boost::json;

namespace rbk::AuthAdv {

namespace {

constexpr size_t kMaxGroupName = 64;
constexpr size_t kMaxGroupNote = 255;

void writeError(Payload& payload, int statusCode, const std::string& message) {
	rbk::Auth::writeJsonError(payload, statusCode, message);
	payload.mime = "application/json";
}

void writeOk(Payload& payload, bj::object json = {}) {
	json["status"] = "ok";
	payload.mime   = "application/json";
	payload.html   = pretty_print(json);
}

void audit(const std::string& text) {
	if (auto& c = conf(); c.audit) {
		c.audit(text);
	}
}

const PrivDef* findPriv(const std::vector<PrivDef>& privs, std::string_view name) {
	const auto it = std::ranges::find(privs, name, &PrivDef::name);
	return it == privs.end() ? nullptr : &*it;
}

void warnUnknownOnce(const std::string& name) {
	static std::mutex            lock;
	static std::set<std::string> seen;
	std::lock_guard              guard(lock);
	if (seen.insert(name).second) {
		qWarning().noquote() << F("aclGroupPriv has a priv this binary does not know: {}", name).c_str();
	}
}

// false = the 400 is written
bool readBody(PMFCGI& status, Payload& payload, bj::object& out) {
	auto decoder = parseJson(status.body);
	if (decoder.failed() || !decoder.json.is_object()) {
		writeError(payload, 400, "the request body must be a JSON object");
		return false;
	}
	out = std::move(decoder.json.as_object());
	return true;
}

std::optional<u32> readId(const bj::object& j, const char* key) {
	const auto* it = j.if_contains(key);
	if (!it || !it->is_int64() || it->as_int64() <= 0 || it->as_int64() > UINT32_MAX) {
		return std::nullopt;
	}
	return static_cast<u32>(it->as_int64());
}

std::string readString(const bj::object& j, const char* key) {
	const auto* it = j.if_contains(key);
	if (!it || !it->is_string()) {
		return {};
	}
	return QString::fromUtf8(it->as_string().data(), static_cast<qsizetype>(it->as_string().size()))
	    .trimmed()
	    .toStdString();
}

// empty = the 400 is written
std::string readGroupName(const bj::object& j, Payload& payload) {
	auto name = readString(j, "name");
	if (name.empty() || name.size() > kMaxGroupName) {
		writeError(payload, 400, F("name is required, at most {} bytes", kMaxGroupName));
		return {};
	}
	return name;
}

std::optional<std::string> groupName(u32 group) {
	auto row = conf().db->queryLine(F("SELECT name FROM aclGroup WHERE id = {}", group));
	if (row.empty()) {
		return std::nullopt;
	}
	return row.rq<std::string>("name");
}

// except: the group being renamed, so a change of case or accent is not a clash with itself
bool nameTaken(const std::string& name, u32 except = 0) {
	auto& db = *conf().db;
	return !db.queryLine(F("SELECT id FROM aclGroup WHERE name = '{}' AND id <> {}", db.escape(name), except)).empty();
}

// nullopt = the answer is written
std::optional<std::pair<u32, std::string>> readExistingGroup(const bj::object& j, const char* key, Payload& payload) {
	const auto group = readId(j, key);
	if (!group) {
		writeError(payload, 400, F("{} is required", key));
		return std::nullopt;
	}
	auto name = groupName(*group);
	if (!name) {
		writeError(payload, 404, "group not found");
		return std::nullopt;
	}
	return std::make_pair(*group, std::move(*name));
}

} // namespace

Conf& conf() {
	static Conf c;
	return c;
}

rbk::Auth::CheckResult check(int priv, Op op) {
	using R = rbk::Auth::CheckResult;
	auto& c = conf();
	if (c.isRoot && c.isRoot()) {
		return R::root;
	}
	if (!c.bitsOfCurrentUser) {
		return R::deny403;
	}
	const auto& bits  = c.bitsOfCurrentUser();
	const auto  index = static_cast<size_t>(priv);
	return index < bits.size() && (bits[index] & static_cast<u8>(op)) != 0 ? R::pass : R::deny403;
}

bool can(int priv, Op op) {
	return check(priv, op) != rbk::Auth::CheckResult::deny403;
}

Bits loadBits(DB& db, const std::vector<PrivDef>& privs, u32 group) {
	size_t size = 0;
	for (const auto& def : privs) {
		size = std::max(size, static_cast<size_t>(def.id) + 1);
	}
	Bits bits(size, 0);
	if (!group) {
		return bits;
	}
	const auto rows = db.query(F("SELECT priv, canRead, canWrite FROM aclGroupPriv WHERE groupId = {}", group));
	for (const auto& row : rows) {
		const auto  name = row.rq<std::string>("priv");
		const auto* def  = findPriv(privs, name);
		if (!def) {
			warnUnknownOnce(name);
			continue;
		}
		u8 mask = 0;
		if (row.rq<u32>("canRead")) {
			mask |= static_cast<u8>(Op::read);
		}
		if (row.rq<u32>("canWrite")) {
			mask |= static_cast<u8>(Op::write);
		}
		bits[static_cast<size_t>(def->id)] = mask;
	}
	return bits;
}

void matrixApi(PMFCGI& status, Payload& payload) {
	(void)status;
	auto& c = conf();

	bj::array privs;
	for (const auto& def : c.privs) {
		bj::object p;
		p["name"]    = def.name;
		p["label"]   = def.label;
		p["audited"] = def.audited;
		privs.emplace_back(std::move(p));
	}

	std::map<u32, bj::object> ticks;
	for (const auto& row : c.db->query("SELECT groupId, priv, canRead, canWrite FROM aclGroupPriv")) {
		const auto name = row.rq<std::string>("priv");
		if (!findPriv(c.privs, name)) {
			continue;
		}
		bj::object cell;
		cell["read"]                          = row.rq<u32>("canRead") != 0;
		cell["write"]                         = row.rq<u32>("canWrite") != 0;
		ticks[row.rq<u32>("groupId")][name] = std::move(cell);
	}

	bj::array groups;
	for (const auto& row : c.db->query("SELECT id, name, note FROM aclGroup ORDER BY id")) {
		const auto id = row.rq<u32>("id");
		bj::object g;
		g["id"]        = id;
		g["name"]      = row.rq<std::string>("name");
		g["note"]      = row.rq<std::string>("note");
		g["protected"] = c.isProtectedGroup && c.isProtectedGroup(id);
		g["members"]   = c.membersOf ? c.membersOf(id) : bj::array{};
		g["ticks"]     = std::move(ticks[id]);
		groups.emplace_back(std::move(g));
	}

	bj::object json;
	json["privs"]  = std::move(privs);
	json["groups"] = std::move(groups);
	writeOk(payload, std::move(json));
}

void setBitApi(PMFCGI& status, Payload& payload) {
	bj::object j;
	if (!readBody(status, payload, j)) {
		return;
	}
	const auto group = readExistingGroup(j, "group", payload);
	if (!group) {
		return;
	}
	auto&      c    = conf();
	const auto priv = readString(j, "priv");
	// checked against the list, so the name is safe in the SQL below
	if (!findPriv(c.privs, priv)) {
		writeError(payload, 400, "unknown priv");
		return;
	}
	const auto op = readString(j, "op");
	if (op != "read" && op != "write") {
		writeError(payload, 400, "op must be read or write");
		return;
	}
	const auto* on = j.if_contains("on");
	if (!on || !on->is_bool()) {
		writeError(payload, 400, "on must be true or false");
		return;
	}
	const std::string_view column = op == "read" ? "canRead" : "canWrite";
	const int              value  = on->as_bool() ? 1 : 0;

	auto       current = c.db->queryLine(F("SELECT {} AS v FROM aclGroupPriv WHERE groupId = {} AND priv = '{}'", column, group->first, priv));
	const bool had     = !current.empty() && current.rq<u32>("v") != 0;

	c.db->query(F("INSERT INTO aclGroupPriv (groupId, priv, {0}) VALUES ({1}, '{2}', {3}) "
	              "ON DUPLICATE KEY UPDATE {0} = {3}",
	              column, group->first, priv, value));

	audit(F("acl: group {} ({}) priv {} {}: {} -> {}", group->first, group->second, priv, op, had ? 1 : 0, value));
	writeOk(payload);
}

void groupCreateApi(PMFCGI& status, Payload& payload) {
	bj::object j;
	if (!readBody(status, payload, j)) {
		return;
	}
	const auto name = readGroupName(j, payload);
	if (name.empty()) {
		return;
	}
	if (nameTaken(name)) {
		writeError(payload, 409, "a group with this name exists");
		return;
	}
	auto&      db   = *conf().db;
	const auto note = readString(j, "note");
	if (note.size() > kMaxGroupNote) {
		writeError(payload, 400, F("note is at most {} bytes", kMaxGroupNote));
		return;
	}
	db.query(F("INSERT INTO aclGroup (name, note) VALUES ('{}', '{}')", db.escape(name), db.escape(note)));
	const auto id = db.lastId();

	audit(F("acl: group {} ({}) created", id, name));
	bj::object json;
	json["id"] = id;
	writeOk(payload, std::move(json));
}

void groupCloneApi(PMFCGI& status, Payload& payload) {
	bj::object j;
	if (!readBody(status, payload, j)) {
		return;
	}
	const auto from = readExistingGroup(j, "from", payload);
	if (!from) {
		return;
	}
	const auto name = readGroupName(j, payload);
	if (name.empty()) {
		return;
	}
	if (nameTaken(name)) {
		writeError(payload, 409, "a group with this name exists");
		return;
	}
	auto& db = *conf().db;
	db.query(F("INSERT INTO aclGroup (name, note) VALUES ('{}', '')", db.escape(name)));
	const auto id = db.lastId();
	db.query(F("INSERT INTO aclGroupPriv (groupId, priv, canRead, canWrite) "
	           "SELECT {}, priv, canRead, canWrite FROM aclGroupPriv WHERE groupId = {}",
	           id, from->first));

	audit(F("acl: group {} ({}) cloned from {} ({})", id, name, from->first, from->second));
	bj::object json;
	json["id"] = id;
	writeOk(payload, std::move(json));
}

void groupRenameApi(PMFCGI& status, Payload& payload) {
	bj::object j;
	if (!readBody(status, payload, j)) {
		return;
	}
	const auto group = readExistingGroup(j, "group", payload);
	if (!group) {
		return;
	}
	const auto name = readGroupName(j, payload);
	if (name.empty()) {
		return;
	}
	if (name == group->second) {
		writeOk(payload);
		return;
	}
	if (nameTaken(name, group->first)) {
		writeError(payload, 409, "a group with this name exists");
		return;
	}
	auto& db = *conf().db;
	db.query(F("UPDATE aclGroup SET name = '{}' WHERE id = {}", db.escape(name), group->first));

	audit(F("acl: group {} renamed: {} -> {}", group->first, group->second, name));
	writeOk(payload);
}

void groupDeleteApi(PMFCGI& status, Payload& payload) {
	bj::object j;
	if (!readBody(status, payload, j)) {
		return;
	}
	const auto group = readExistingGroup(j, "group", payload);
	if (!group) {
		return;
	}
	auto& c = conf();
	if (c.isProtectedGroup && c.isProtectedGroup(group->first)) {
		writeError(payload, 409, "a seeded group cannot be deleted");
		return;
	}
	if (c.membersOf && !c.membersOf(group->first).empty()) {
		writeError(payload, 409, "the group has members");
		return;
	}
	c.db->query(F("DELETE FROM aclGroupPriv WHERE groupId = {}", group->first));
	c.db->query(F("DELETE FROM aclGroup WHERE id = {}", group->first));

	audit(F("acl: group {} ({}) deleted", group->first, group->second));
	writeOk(payload);
}

} // namespace rbk::AuthAdv
