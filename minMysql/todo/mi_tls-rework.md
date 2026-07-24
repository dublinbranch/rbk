# mi_tls / connection-handling rework

Written 2026-07-24, right after fixing the `DB::~DB()` segfault at exit.
Context for whoever (me) picks this up cold months from now.

## Verdict first: do NOT replace thread-affinity with a classic connection pool

This was seriously considered and rejected. The per-thread connection model is the
right one for this codebase. What is shaky is the `mi_tls` *implementation*, not the
idea. Details below so the decision does not get re-litigated from scratch.

### Why thread-affinity is correct here

Facts checked in digitalSpine on 2026-07-24:

- Only **2 live DB instances**, both immortal globals: `mainDBVal` and `runnableDB`
  (`config.cpp:21,23`). `TTLCache` is the only class embedding a `DB` by value
  (`ttlcache.h:19`) and it is not used anywhere in digitalSpine.
- **~15 long-lived threads, no churn**: 10 Beast workers (`config.cpp:159` sets
  `http.worker = 10`), the MQTT `ioc` workers, plus `cacheManager`, `simulation`,
  `calculatePerTickTotals` and the shelly `pollLoop`.
- **No DB operation ever spans a suspension point.** Beast is callback-async but each
  handler runs to completion on one thread. The MQTT side *is* a coroutine, but
  `mqttMessage()` (`MQTT/mqttclient.cpp:88`) contains no `co_await`.

That last point is the whole argument. A thread-local connection is correct precisely
when a logical DB operation never migrates threads mid-flight. It holds everywhere today.

A pool would buy decoupling of connection count from thread count. That is ~15 threads
x 2 DBs = ~30 connections against a default `max_connections` of 151, i.e. worth nothing.
It would cost a refactor of every call site, because call sites read state *after* the
query with no handle in hand (`mainDB->state->lastError`, `lastId()`, `getAffectedRows()`).
The alternative to threading a lease object everywhere is a thread-local "current lease",
which is this same design with extra indirection.

### What would flip the verdict

Revisit only if one of these becomes true:

1. DB work moves into coroutines that `co_await` mid-transaction. The moment a query and
   its `lastError` read can land on different threads, TLS is broken and an explicit
   lease handle becomes mandatory.
2. Fewer connections than threads are wanted (thread count grows a lot, or MariaDB's
   `max_connections` gets tight).
3. Short-lived / dynamically spawned threads start doing DB work, making connection churn
   a real cost.

## The bug that was already fixed (do not re-introduce)

`MITLS.h`: the `alive` guard was a **member of `Repo` assigned from `~Repo()`**. That is a
dead store the optimizer legally deletes, and GCC did delete it, so the guard was never
compiled in at all. `exit()` runs `__call_tls_dtors()` *before* the atexit handlers, so the
main thread's `repo` dies before the global `mainDBVal`; `~DB` then walked a freed
`unordered_map` bucket chain.

Fixed by moving the flag into its own function-local, **trivially destructible**
`thread_local` (`static bool& alive()`): no destructor is ever registered for it, so it
stays readable through static destruction.

Rule: **a flag that reports "this object is dead" must live outside that object.**

How it was proven, if it ever regresses — all three signs together:
- no store to the flag's offset in `~Repo`'s disassembly
  (`gdb -batch -ex "disassemble 'mi_tls_repository<bool>::Repo::~Repo()'"`)
- the flag still reads `1` in memory after `~Repo` returned
- the faulting instruction is a bucket-chain walk (`mov 0x8(%rax),%rdi` with garbage `rax`)

## Remaining defects, in priority order

1. **Six type-keyed maps per DB.** Each `DB` carries 6 `mi_tls` members (`noFetch`,
   `skipWarning`, `state`, `affectedRows`, `connPool`, `signalMask`, see `min_mysql.h`),
   and each instantiates its own `mi_tls_repository<T>::repo` thread_local map. A single
   query pays a TLS-wrapper call + hash + bucket probe several times over, for state that
   is conceptually one struct. This is the main reason to do the rework.

2. **`remove()` only cleans the current thread.** `~mi_tls` cannot reach other threads'
   repos, so a destroyed DB leaves entries in every other thread's map keyed by its
   address. Harmless today because both DBs are immortal, but it is a live landmine: a
   dynamically created DB leaks one connection per thread, and a new DB allocated at a
   recycled address inherits the dead one's connection on every other thread. If
   `TTLCache` ever gets used, this bites immediately.

3. **The explicit-specialization list in `MITLS.cpp`** must be hand-edited for every new
   `T` or you get a link error. Those specializations are also not declared in the header
   before first use, which is technically IFNDR; it only happens to link.

4. **Shutdown ordering fragility** is inherent to a static-duration object owning
   thread-local state. Patched, not eliminated.

## Plan of action

Collapse the six type-keyed maps into one per-thread struct. Contained to
`min_mysql.h` / `min_mysql.cpp` / `MITLS.h`, and **no call site has to change** —
`state->lastSQL` and friends keep working through the same accessors.

```cpp
struct DB::PerThread {
    StMysqlPtr    conn;
    InternalState state;
    u64  affectedRows = 0;
    int  signalMask   = 0;
    bool noFetch = false, skipWarning = false;
};
PerThread& DB::mine() const;   // one lookup per query instead of six
```

With only two immortal instances, go further: give each `DB` a small dense index assigned
at construction and make storage a plain `thread_local PerThread[N]`. That is zero hashing
on the hot path and drops the map (and defect 2) entirely.

This kills defects 1, 3 and 4 outright and reduces 2 to a single-entry cleanup.

Keep while doing it:
- `~DB` must stay a no-op. It must never call `closeConn()` — that touches the *current*
  thread's connection, and during shutdown that thread's TLS may already be gone.
- Per-thread `St_mysqlW` must still be closed when a thread exits (that is what releases
  connections); today `~Repo` does it.
- Whatever liveness guard survives must stay outside the object it describes.

## Verification

`build/test/digitalSpine_tests` and the standalone suites all exit 0 as of the fix — that
is the regression signal, exit code 139 is the failure. Run `ctest` from `build/`.

Note `appliance_map_index_test`, `appliance_normalize_test`, `id_csv_test` (link error,
`undefined reference to 'tv'`) and `appliance_cache_lookup_test` (uses `mainDB->state.foo`
with a `.` where `state` is an `mi_tls`, needs `state->`) are broken for unrelated
pre-existing reasons. Verified by stashing the fix and rebuilding. Do not chase those.
