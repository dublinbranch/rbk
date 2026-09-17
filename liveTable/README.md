# LiveTable

An in-memory copy of a DB table, kept current by the DB. Header only: `liveTable.h`.

```cpp
struct Row {
	static constexpr std::string_view table = "myTable";
	u64  id = 0;                       // must be exactly u64
	void fromSql(const SqlRowV2& r);
	bool alive() const;                // false = erase from the map (soft delete flag)
	bool operator==(const Row&) const; // equal rows skip replace
	// optional: void adopt(const Row& old);  carry live (non-SQL) state across replace
};

struct ByName {};
struct MyOwner;   // the only class that may call the private upsert / erase

rbk::LiveTable<Row, MyOwner,
    boost::multi_index::hashed_unique<boost::multi_index::tag<ByName>, rbk::ptrMember<&Row::name>>> t;
```

The table needs `updatedAt TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3)` with an index.

| Call | Does |
|---|---|
| `loadAll(db)` | boot: `SELECT NOW(3)` → cursor, then full `SELECT … WHERE aliveWhere` |
| `refresh(db)` | delta `WHERE updatedAt >= cursor - overlapSeconds`; upsert, erase `!alive()`; returns erased rows |
| `reconcile(db)` | full `SELECT`; upsert; erase ids not in the result; returns erased rows |
| `get(id)`, `find<Tag>(key)`, `findAll<Tag>(key)`, `all()` | `sharedLock` only; return `shared_ptr` copies |
| `admitNew` | optional `bool(Row&)` for a row not yet in the map: `false` skips it; may fill live state |

Rules:

- Rows are `shared_ptr<const Row>`. Never change a row in place. A held `Ptr` stays valid after replace / erase.
- Index extractors must be `rbk::ptrMember<&Row::field>`. `boost::multi_index::member` / `key` do not compile on `shared_ptr<const Row>`.
- Lock order: `writeGate` → `map.uniqueLock()`. Never wait on `writeGate` while holding a map lock.
- An owner write (INSERT / UPDATE + map) holds `writeGate` across the DB statement and the map change, so a concurrent `refresh` / `reconcile` cannot apply an older snapshot on top of it.
- A hard `DELETE` is not seen by `refresh`; only `reconcile` drops it. Prefer a soft flag read by `alive()`.
- rbk starts no thread. The app calls `refresh` / `reconcile` on its own timer.
