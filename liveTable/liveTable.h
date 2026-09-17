#ifndef RBK_LIVETABLE_H
#define RBK_LIVETABLE_H

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/mapExtensor/ThreadSafeMultiIndex.hpp"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlrowv2.h"
#include "rbk/number/intTypes.h"
#include "rbk/QStacker/exceptionv2.h"

#include <QDebug>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * LiveTable — an in-memory copy of a DB table, kept current by the DB.
 * See README.md next to this file. Design notes: PPPLC todo/live-table-sessions.md.
 *
 * - Rows are std::shared_ptr<const Row>. A row is never changed in place: refresh
 *   replaces the pointer. A reader that holds a Ptr keeps a stable copy without a lock.
 * - loadAll()   : full load at boot (WHERE aliveWhere).
 * - refresh()   : delta, WHERE updatedAt >= lastTs - overlap. Upsert, erase !alive().
 * - reconcile() : full load, upsert, erase ids that are not in the result.
 * - rbk starts no thread. The app calls refresh / reconcile.
 *
 * Locks (mandatory order): writeGate -> map.uniqueLock(). Never wait on writeGate
 * while holding a map lock. get / find take map.sharedLock() only.
 */

namespace rbk {

// Key extractor for a container of shared_ptr<const Row>. boost member<Row, T, &Row::f>
// and key<&Row::f> do not compile there: the non-const member returns T& from a const Row.
template <auto PM>
struct ptrMember;

template <class C, class T, T C::*PM>
struct ptrMember<PM> {
	using result_type = T;
	const T& operator()(const std::shared_ptr<const C>& p) const {
		return (*p).*PM;
	}
};

template <class T>
concept LiveRow = requires(T& t, const SqlRowV2& r) {
	{ T::table } -> std::convertible_to<std::string_view>;
	requires std::same_as<decltype(T::id), u64>;
	t.fromSql(r);
	{ std::as_const(t).alive() } -> std::same_as<bool>;
	requires std::equality_comparable<T>;
};

// Optional: carry live state (not from SQL) from the old row to its replacement.
template <class T>
concept LiveRowAdopt = requires(T& t, const T& old) { t.adopt(old); };

struct ById {};

template <LiveRow Row, class Owner, class... ExtraIndexes>
class LiveTable {
      public:
	using Ptr       = std::shared_ptr<const Row>;
	using Rows      = std::vector<Ptr>;
	using Container = boost::multi_index_container<
	    Ptr,
	    boost::multi_index::indexed_by<
	        boost::multi_index::hashed_unique<boost::multi_index::tag<ById>, ptrMember<&Row::id>>,
	        ExtraIndexes...>>;

	/** Boot: SELECT NOW(3) -> lastTs, then the full SELECT. Replaces the map. */
	void loadAll(DB& db) {
		std::lock_guard gate(writeGate);
		const auto      nextTs = dbNow3(db);
		auto            fresh  = fetch(db, fullWhere());
		{
			auto lk = map.uniqueLock();
			map.container.clear();
			for (auto& row : fresh) {
				if (row->alive()) {
					upsertLocked(std::move(row));
				}
			}
		}
		lastTs = nextTs;
	}

	/** Delta. Returns the rows it erased (alive() became false), for the owner to act on after the locks. */
	Rows refresh(DB& db) {
		std::lock_guard gate(writeGate);
		if (lastTs.empty()) {
			// No cursor: the delta would be WHERE updatedAt >= NULL and silently match nothing, forever.
			throw ExceptionV2(F("LiveTable<{}>::refresh called before loadAll", Row::table));
		}
		const auto      nextTs = dbNow3(db);
		auto            fresh =
		    fetch(db, F(" WHERE updatedAt >= '{}' - INTERVAL {} SECOND", db.escape(lastTs), overlapSeconds));
		Rows erased;
		{
			auto lk = map.uniqueLock();
			for (auto& row : fresh) {
				if (row->alive()) {
					upsertLocked(std::move(row));
				} else if (auto old = eraseLocked(row->id)) {
					erased.push_back(std::move(old));
				}
			}
		}
		lastTs = nextTs;
		return erased;
	}

	/** Full reload: upsert every row, erase ids that are not in the result. Returns the erased rows. */
	Rows reconcile(DB& db) {
		std::lock_guard         gate(writeGate);
		auto                    fresh = fetch(db, fullWhere());
		std::unordered_set<u64> seen;
		seen.reserve(fresh.size());
		Rows erased;
		{
			auto lk = map.uniqueLock();
			for (auto& row : fresh) {
				if (!row->alive()) {
					continue;
				}
				seen.insert(row->id);
				upsertLocked(std::move(row));
			}
			std::vector<u64> gone;
			for (const auto& p : map.container) {
				if (!seen.contains(p->id)) {
					gone.push_back(p->id);
				}
			}
			for (auto id : gone) {
				if (auto old = eraseLocked(id)) {
					erased.push_back(std::move(old));
				}
			}
		}
		return erased;
	}

	[[nodiscard]] Ptr get(u64 id) const {
		return find<ById>(id);
	}

	template <class Tag, class Key>
	[[nodiscard]] Ptr find(const Key& key) const {
		auto        lk  = map.sharedLock();
		const auto& idx = map.container.template get<Tag>();
		if (auto it = idx.find(key); it != idx.end()) {
			return *it;
		}
		return nullptr;
	}

	template <class Tag, class Key>
	[[nodiscard]] Rows findAll(const Key& key) const {
		auto        lk  = map.sharedLock();
		const auto& idx = map.container.template get<Tag>();
		auto [b, e]     = idx.equal_range(key);
		return Rows(b, e);
	}

	/** Snapshot of every row. The pointers stay valid after the lock is released. */
	[[nodiscard]] Rows all() const {
		auto lk = map.sharedLock();
		return Rows(map.container.begin(), map.container.end());
	}

	ThreadSafeMultiIndex<Container> map;
	mutable std::mutex              writeGate;
	i64                             overlapSeconds = 60;
	// loadAll + reconcile only. Not used by the delta refresh, which must see rows that became dead.
	std::string aliveWhere;
	std::string selectCols = "*";

	// Called for a row whose id is not yet in the map, just before insert. Return false to skip it
	// (e.g. already expired). May fill live state. Map uniqueLock is held: cheap, no DB, no other lock.
	std::function<bool(Row&)> admitNew;
	// Test seam: runs between the SELECT and the apply, writeGate held.
	std::function<void()> testHookAfterSelect;

      private:
	friend Owner;

	// Owner only. Caller holds writeGate (the DB write it just did must not race a refresh).
	void upsert(std::shared_ptr<Row> row) {
		auto lk = map.uniqueLock();
		upsertLocked(std::move(row));
	}

	// Owner only. Caller holds writeGate.
	Ptr erase(u64 id) {
		auto lk = map.uniqueLock();
		return eraseLocked(id);
	}

	void upsertLocked(std::shared_ptr<Row> row) {
		auto& idx = map.container.template get<ById>();
		auto  it  = idx.find(row->id);
		if (it == idx.end()) {
			if (admitNew && !admitNew(*row)) {
				return;
			}
			if (!map.container.insert(std::move(row)).second) {
				qWarning().noquote() << F("LiveTable<{}>: insert rejected by a unique index", Row::table);
			}
			return;
		}
		if (**it == *row) {
			return;
		}
		if constexpr (LiveRowAdopt<Row>) {
			row->adopt(**it);
		}
		const auto id = row->id;
		if (!idx.replace(it, std::move(row))) {
			qWarning().noquote()
			    << F("LiveTable<{}>: replace of id {} rejected by a unique index, reconcile will retry", Row::table, id);
		}
	}

	Ptr eraseLocked(u64 id) {
		auto& idx = map.container.template get<ById>();
		auto  it  = idx.find(id);
		if (it == idx.end()) {
			return nullptr;
		}
		Ptr old = *it;
		idx.erase(it);
		return old;
	}

	std::string fullWhere() const {
		return aliveWhere.empty() ? std::string() : " WHERE " + aliveWhere;
	}

	static std::string dbNow3(DB& db) {
		auto res = db.queryV2("SELECT NOW(3) AS ts");
		return res.at(0).template rq<std::string>("ts");
	}

	std::vector<std::shared_ptr<Row>> fetch(DB& db, const std::string& where) {
		auto res = db.queryV2(F("SELECT {} FROM `{}`{}", selectCols, Row::table, where));
		if (testHookAfterSelect) {
			testHookAfterSelect();
		}
		std::vector<std::shared_ptr<Row>> out;
		out.reserve(res.size());
		for (const auto& r : res) {
			auto row = std::make_shared<Row>();
			row->fromSql(r);
			out.push_back(std::move(row));
		}
		return out;
	}

	std::string lastTs;
};

} // namespace rbk

#endif // RBK_LIVETABLE_H
