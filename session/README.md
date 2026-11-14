# session

Login sessions in the DB, kept in memory by `rbk::LiveTable` (`../liveTable/`). No DB read and no
DB write on a request. From the PPPLC CMMS (`CMMS/sessionStore.*`, `CMMS/cloud/sessionAdmin.*`);
digitalSpine uses it since 2026-11-14 (`class/login_store.*`, its `todo/accountLevels.md` 16, 17).

| File | What |
|---|---|
| `sessionStore.h/.cpp` | `rbk::SessionStore`: the `session` table, create, resolve, revoke, refresh, reconcile, cleanup |
| `sessionAdmin.h/.cpp` | `rbk::SessionAdmin`: JSON routes to list and end sessions; the page is the project's |

## The user side stays in the project

`SessionStore` is abstract. A project subclass keeps its own user table (a `LiveTable` on its user
row: `CmmsUser`, the spine's `account`) and fills four hooks:

```cpp
class MyStore : public rbk::SessionStore {
      public:
	rbk::LiveTable<MyUser, MyStore /*, indexes */> users;
      protected:
	void      loadUsers(DB& db) override { users.loadAll(db); }
	void      refreshUsers(DB& db) override { users.refresh(db); }
	void      reconcileUsers(DB& db) override { users.reconcile(db); }
	UserState userState(u64 id) const override { // memory only: it runs on each request
		auto u = users.get(id);
		return !u ? UserState::unknown : u->enabled ? UserState::ok : UserState::disabled;
	}
};
```

- `ok`: the sessions of that user resolve.
- `disabled`: refused, and `refresh` revokes them in the DB.
- `unknown` (not in memory): refused, never revoked. A bad load must not end every session.

## Use

| Call | When |
|---|---|
| `boot(db)` | before the first request |
| `maintain(db)` | every few seconds, from one thread: `refresh`; `reconcile` every 10 min; `cleanup` once a day (also at the first call) |
| `create(db, userId, ttl, ip, ua)` | after the password check, with the user already in memory. Returns the 128-bit sid; the cookie is `sidToHex(sid)` |
| `resolve(cookie)` | each request; memory only. `touch()` keeps last seen and last IP in memory |
| `revoke` / `revokeRow` / `revokeUser` | logout, the admin page, a disabled user, a new password |

After a write of the panel on a user row, re-read that row at once with `users.reload(db, id)`
(`LiveTable::reload`), and end its sessions if it can no longer log in. An edit made in the DB by
hand acts at the next `refresh`.

The table and the memory hold `sessionKey(sid)` (SHA-256), never the sid: a reader of the table
(a dump, a read-only grant) cannot rebuild a cookie. The CMMS table holds the raw sid: on its switch
to this store every CMMS login logs in once more.

Sessions survive a restart: they are DB rows. A session row deleted by hand is caught by a count
at each `refresh`. A user deleted by hand is the project's: its `refreshUsers` can do the same
count (the spine does); otherwise `reconcile` sees it. Prefer the `enabled` flag.

The DDL of the table is in the comment of `sessionStore.h`.

## Admin routes

Fill `rbk::SessionAdmin::conf()` (store, db, `isAdmin`, `currentUserId`, `currentSid`,
`describeUser`, `audit`) and route the handlers. Each handler also checks its caller, so a wrong
route fails closed. The answers never carry a sid, only `session.id`.
