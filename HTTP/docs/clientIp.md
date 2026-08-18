# Client IP behind a reverse proxy (`rbk::Http`)

`HTTP/clientIp.h` answers one question: which address do we attribute a request to, when the
request reached us through nginx? The answer normally becomes a DB filter, a rate limit key or
a log line, so it has to satisfy two properties:

1. The client must not be able to choose it.
2. It must be an address, not text that looks like one.

Both were broken in digitalSpine until 18 Aug 2026. The listener read the **first** element of
`X-Forwarded-For`, did not check the peer, did not check the shape, and the value went into a
query with `fmt::format`. That gave an unauthenticated SQL injection on a websocket endpoint,
non-blind, with `CLIENT_MULTI_STATEMENTS` on. This component exists so the next project does
not rebuild the same hole.

## The three rules

### Trust the peer before you trust the header

Any client can send `X-Forwarded-For`. The header is only evidence when it came from a proxy
we put there ourselves. `isTrustedProxy()` accepts loopback always (nginx usually runs on the
same host) and otherwise requires an exact match in `BeastConf::trustedProxies`. An untrusted
peer gets its headers ignored and its socket address used.

Unparsable entries in `trustedProxies` are skipped. A typo must never widen the match.

### Take the rightmost element, not the leftmost

The usual nginx config is

```nginx
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
```

`$proxy_add_x_forwarded_for` **appends** the peer nginx saw to whatever the client already
sent. So a client sending `X-Forwarded-For: EVIL` arrives as `EVIL, 1.2.3.4`. The last element
is the only one the client could not write; everything to its left is client input. Reading
left to right is the intuitive reading and the wrong one.

`X-Real-IP` (`$remote_addr`) holds a single value, so rightmost == only, and the same function
handles it.

### Parse strictly, and pass on the canonical form

`parseAddress()` rejects anything `boost::asio::ip::make_address` rejects.
`normalizeAddress()` then folds a v4-mapped IPv6 address (`::ffff:a.b.c.d`, any case or
hex form Asio accepts) to dotted IPv4. `clientIp()` returns that spelling on both paths:
the socket peer, and a header from a trusted proxy. On a dual-stack listener the same host
is `::ffff:203.0.113.7` direct and `203.0.113.7` via nginx `$remote_addr`; without the fold
those are two rate-limit keys and two `{ip}_access.log` files.

Callers get `std::nullopt`, never a half-checked string.

## Why Boost.Asio and not `QHostAddress`

Qt is already linked in most of these projects, so `QHostAddress` looks like the free option.
It is the wrong parser for this job. Measured on Qt 6.8.3 and the Boost this tree builds
against, feeding the same strings to both:

| input | `boost::asio::make_address` | `QHostAddress::setAddress` |
|---|---|---|
| `203.0.113.7` | ok | ok |
| `2001:db8::1` | ok | ok |
| `::ffff:203.0.113.7` | ok | ok |
| `example.com` | reject | reject |
| `1.2.3.4.5` | reject | reject |
| `1.2.3.256` | reject | reject |
| `' OR 1=1 --` | reject | reject |
| `" 1.2.3.4"` (leading space) | reject | **ok** → `1.2.3.4` |
| `"1.2.3.4 "` (trailing space) | reject | **ok** → `1.2.3.4` |
| `1.2.3` | reject | **ok** → `1.2.0.3` |
| `2130706433` | reject | **ok** → `127.0.0.1` |
| `0x7f.0.0.1` | reject | **ok** → `127.0.0.1` |
| `127.000.000.001` | reject | **ok** → `127.0.0.1` |

The bottom six rows are `inet_aton` legacy parsing: decimal, hex and octal spellings of the
same address, plus the three-part shorthand. Two consequences, both bad here:

- **One address gets many spellings.** That is the standard way an allowlist or a
  deny-by-value check is walked past. A validator sitting in front of a SQL sink is exactly
  where you do not want it.
- **The value changes under you.** `1.2.3` comes back as `1.2.0.3`. A parser that returns a
  *different* address from the one it was handed is wrong for a function whose output becomes
  a stored key.

There is also a plain engineering reason. The peer arrives as
`boost::asio::ip::address` from `socket.remote_endpoint()`, in code that is already Asio and
Beast. Going through Qt means `address → std::string → QString → QHostAddress` on every
request, and it pulls in QtNetwork, a module several of these projects do not otherwise link.

One difference that is not a defect either way: an IPv6 scope id. Asio drops it
(`fe80::1%eth0` → `fe80::1`), Qt keeps it. Neither is reachable through a public proxy.

### Where Qt would win

CIDR. `QHostAddress::parseSubnet("10.0.0.0/8")` plus `isInSubnet()` is three lines and treats
v4 and v6 the same. Boost needs `make_network_v4` / `make_network_v6` on separate branches
plus `canonical()`, about fifteen. Both give identical answers, verified on `10.0.0.0/8`,
`/32`, `192.168.1.0/24`, `2001:db8::/32` and on garbage input.

`trustedProxies` is **exact addresses only** today, which covers "nginx on one known host". If
a proxy pool ever makes CIDR worth it, the right split is to use Qt's `parseSubnet` for the
config side only — `isTrustedProxy()` reads configuration, not attacker input, so the loose
parsing is a config footgun there rather than a hole — and keep `make_address` in
`parseAddress()` and `rightmostForwardedFor()`, which do read attacker input.

## Using it

```cpp
#include "rbk/HTTP/clientIp.h"

// inside a websocket upgrade hook or a request handler
const auto ip = rbk::Http::clientIp(socket, req, conf);   // conf is your BeastConf
```

Config, same JSON path in every project that serialises `BeastConf`:

```json
"http": { "trustedProxies": ["10.0.0.2"] }
```

Absent or `null` means loopback only.

For a value that arrives some other way (a GET parameter, a device field, a DB column) use
`normalizeAddress()` directly and treat `nullopt` as "no address" — do not fall back to
passing the raw text through.

Tests: `rbk/test/clientIp.cpp`, in the `rbk_tests` target.
