# minCurl v2: one call with an options struct, TLS verification on by default

Written 2026-11-11, from the PPPLC review (finding B2 in
`PPPLC/todo/architecture-review-2026-09-23.md`).

## Status (2026-11-11): implemented, uncommitted

Done, in `mincurl.h` / `mincurl.cpp`:

- `CurlOpt`, `urlGetV2`, `urlPostV2`, `urlPutV2`.
- One core, `curlPerform`. The v1 calls are now wrappers over it with verify off.
- v1 behaviour kept on purpose, because other projects' use of it could not be checked:
  - GET sets no method (clickhouse posts through it);
  - GET with an empty url keeps the handle's url;
  - a borrowed handle keeps its timeouts;
  - an empty-body POST is a bare `CUSTOMREQUEST`;
  - `redirectUrl` is filled only for a GET with a caller handle;
  - GET reports `httpCode` only on success;
  - the log lines keep their wording.
- Fixed for v1 and v2: `result.ip`, PUT `httpCode`, `POSTFIELDSIZE`, the dangling pointers on a
  borrowed handle, and `QSWriter` reading past the header data (it ignored curl's length).
- Fixed: the thread status was never "cURL" during a v1 GET. The `ResetOnExit` lived inside the
  `if (LTS)` block, so it restored the old state immediately.

Tests: `test/curltls.cpp` (in `rbk_tests`, added only when `RBK_WITH_MINCURL` is on). To build a
`rbk_tests` that links, configure with `-DRBK_WITH_MINCURL=ON -DRBK_WITH_BOOST_BEAST=ON -DRBK_WITH_APCU=ON`.
With the default options (all off), `clientIp.cpp` and `loginLimiter.cpp` do not link either. That was
already the case before this change. In that build all 6 curl_tls cases pass (2026-11-11). 7
`filefunction` revision checks fail; that predates this change: the test is from 2026-05, and
`filefunction.cpp` changed in 2026-09/10. The tests run against a local
`openssl s_server` with a throwaway certificate:
- verify on rejects it, for GET and POST;
- a pinned `caFile` accepts it;
- the wrong host name is rejected;
- verify off works, and a reused handle carries nothing from the earlier call;
- v1 still connects with verify off.

PPPLC: `CMMS::postJson` uses `urlPostV2` with verify on.

Not done:

- Deleting the `urlGetContentV3` stub. Other projects could not be grepped to confirm nothing names it.
- `curlpp.cpp:256` `VERIFYHOST` 1 → 2.
- Deprecating the v1 calls.
- Moving PPPLC's `https://seisho.us/ip.php` lookup (`function.cpp`) to v2. That host sends no
  intermediate certificate (`openssl s_client`: verify error 21), so it fails verification until
  nginx serves the full chain. `cmms.seisho.us` has the same problem.

## Why

`urlGetContent2`, `urlPostContent` and `urlPutContent` set `CURLOPT_SSL_VERIFYPEER = 0`
and `CURLOPT_SSL_VERIFYHOST = 0` inside every call (`mincurl.cpp:234-235`, `:279-280`,
`:328-329`). They set these after any option the caller put on its own handle, so a
caller cannot turn verification back on. Every HTTPS call through minCurl is therefore
open to a man in the middle.

PPPLC is the caller that needs a fix. Every edge→cloud POST (sync push/pull,
provisioning, fault report, telemetry) goes through `CMMS::postJson`
(`PPPLC/CMMS/core/jsonPost.cpp`), which calls `urlPostContent`. The cloud certificate is
valid: the PPPLC WebSocket client verifies peer and host name against the same server and
works. So the verification being off is not needed. It is only hard-coded.

Adding a 5th bool to `urlPostContent(url, post, quiet, curl)` is the wrong fix. Use one
options struct, as `ExecuteOpt` does for `execute()`, so later options (headers, CA file,
connect timeout, ...) are one new field, not a new signature.

## Proposed API

```cpp
struct CurlOpt {
	bool        verifyTls         = true;  // VERIFYPEER = 1, VERIFYHOST = 2
	std::string caFile;                    // empty = system store; for a private CA, pin it here instead of turning verify off
	long        timeoutSec        = 60;    // same default the v1 calls use
	long        connectTimeoutSec = 0;     // 0 = curl default
	std::vector<std::string> headers;      // "Content-Type: application/json", ...
	bool        okNeeds2xx        = false; // true: ok = transport ok AND 2xx (PPPLC postJson does this by hand)
	bool        quiet             = false;
	bool        LTS               = true;  // localThreadStatus bookkeeping, as in urlGetContent2
	CurlKeeper* curl              = nullptr; // reuse a handle (keep-alive, cookies); nullptr = own handle
};

CurlCallResult urlGetV2(const QByteAdt& url, const CurlOpt& opt = {});
CurlCallResult urlPostV2(const QByteAdt& url, const QByteAdt& body, const CurlOpt& opt = {});
CurlCallResult urlPutV2(const QByteAdt& url, const QByteAdt& body, const CurlOpt& opt = {});
```

Choose the names you want, with two limits:

- Do not overload `urlGetContent2(url, bool quiet, ...)` with a `CurlOpt` version.
  `urlGetContent2(url, {})` would pick the `bool` overload without a warning. An empty
  `{}` converts to `bool` by an identity conversion, which beats the user-defined
  conversion to `CurlOpt`.
- `class urlGetContentV3` (`mincurl.h`, end of file) is an unused stub: its only member
  is a private static, so nothing can call it. Delete it, do not reuse the name.

## Implementation

1. **One core function.** `curlPerform(method, url, body, opt)` in `mincurl.cpp`. Today the
   three calls are three copies of the same ~40 lines, and they have already drifted apart
   (see the bugs below).
2. **The v1 calls become thin wrappers** with `verifyTls = false` and their current
   `quiet` / timeout / handle behaviour, so digitalSpine, googleAdsListener and the other
   v1 callers do not change. Mark them `[[deprecated("use urlPostV2")]]` later, once callers
   have moved.
3. **Set everything on every call.** The core always sets every option it owns, on its own
   handle and on a borrowed one: the method (`HTTPGET` / `POST` + `POSTFIELDSIZE` /
   `CUSTOMREQUEST "PUT"`), both verify options, timeouts, headers. curl options stay set on a
   handle, so an option set by an earlier call must never decide this one.
4. **Leave nothing pointing at the stack.** Before returning, reset the pointers the call
   set on a borrowed handle: `ERRORBUFFER`, `WRITEDATA`, `HEADERDATA`, `POSTFIELDS` and the
   header list.
5. **Clear the self-signed error only when verify is off.** v1 clears the error text
   "self-signed certificate in certificate chain". With `verifyTls = true` this is the real
   error and must be kept.
6. **Qt 5.15.** The PPPLC edge builds rbk with Qt 5.15 (see the 2026-10-22 `QTimeZone::UTC`
   break). Use no Qt ≥ 6 API in the new code.

## Bugs in the v1 code, found while reading it (fix in the core, so v1 gets them too)

- `urlGetContent2`: `result.ip.fromLocal8Bit(ip);` drops its result (`fromLocal8Bit` is
  static), so `result.ip` is always empty on success. Should be
  `result.ip = QString::fromLocal8Bit(ip);`.
- `urlGetContent2`: `CURLINFO_EFFECTIVE_URL` is read from `curl` (the caller's handle, can
  be `nullptr`), not `useMe`. When no handle is passed, `redirectUrl` is never filled.
- `urlPutContent`: never reads `CURLINFO_RESPONSE_CODE`, so `httpCode` is always 0.
- `urlPostContent` / `urlPutContent`: `POSTFIELDS` without `POSTFIELDSIZE`, so curl uses
  `strlen()` and a binary body is cut at the first NUL byte.
- `urlGetContent2` never sets `HTTPGET`. If a reused handle did a POST before, the GET is
  sent as a POST, with `POSTFIELDS` still pointing at the earlier call's dead buffer.
- All three: `ERRORBUFFER` on a borrowed handle keeps pointing at the dead `errbuf` stack
  array after return. A later `curl_easy_perform` on that handle that does not set its own
  buffer writes into the stack.
- `curlpp.cpp:256` sets `VERIFYHOST` to `ssl_verifier` (1). Since curl 7.66, 1 means the
  same as 2, but write 2.

## Test

- Unit test: the options struct defaults (`verifyTls` true, timeout 60).
- Manual, since it needs the network: `https://self-signed.badssl.com/` and
  `https://wrong.host.badssl.com/` must fail with `CURLE_PEER_FAILED_VERIFICATION` (60)
  with verify on, and pass with verify off. A normal site must pass with verify on.
- After the change, PPPLC: `CMMS::postJson` moves to `urlPostV2` with verify on (it is HTTPS
  edge→cloud. The cloud→washer HTTP wake is plain `http://` over the VPN, so verify does
  not apply). Check that the aarch64 washer image has a CA store (`ca-certificates`) and
  that the configured cloud URL is the certificate's host name, not an IP. Then run
  `CMMS/test/run.sh all`.
