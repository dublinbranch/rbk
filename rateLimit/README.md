# rateLimit

Rate limits in memory, no DB, no Qt. From digitalSpine (its `todo/accountLevels.md` 16.3, 18).

| What | |
|---|---|
| `rbk::Limits` | four numbers: requests a minute, burst, busy ms a minute, in flight. A rate of 0 = no limit |
| `rbk::Bucket` | one token bucket on the steady clock: `take`, `positive`, `debit` (with a floor), `secondsUntil` |
| `rbk::RateState` | one limited key (a login, an address): the request bucket, the busy bucket, the requests in flight, the refusals. Thread safe, one leaf mutex |

## A request

```cpp
if (!state.enter(limits.inFlight)) refuse(1);              // a slot is held from here
rbk::RateState::Why why;
if (auto s = state.admit(rbk::steadyNs(), limits, why)) {   // busy first, then a request token
	state.leave();
	refuse(s);                                              // 429, Retry-After: s
}
// ... the request ...
state.debit(rbk::steadyNs(), limits, wallMs);               // every ms, down to -busyMsPerMin
state.leave();
```

- The busy bucket is debited after the fact: a request admitted with a little time left still runs.
  The level can go below 0 (a debt), never below one minute of refill, so a login that ran long, or
  waited on a stalled DB, waits at most about a minute after its request ended.
- The busy bucket is tested before the request bucket, so a request refused for busy takes no token.
- `Payload::scope` (`../HTTP/Payload.h`) is the place for the object that holds the slot and does
  the debit: it is destroyed once at the end of every request, on every path.
- `rbk::AuthAdv::Group::limits` keeps the limits of a group (columns of `aclGroup`), edited with
  `AuthAdv::groupLimitsApi`.
