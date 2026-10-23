# Security review: HTTP, SSE, MCP, WebSocket

**Repo:** [ruoka/net4cpp](https://github.com/ruoka/net4cpp)  
**Reviewed:** v3.1.0, commit `55d6bca`  
**Date:** 2026-10-23  
**Status:** Findings only. Fixes land on separate branches; #1 is on `fix/connection-slot-exhaustion`.

## Scope and method

Every module under `net/` was read, with most attention on `net-http_server.c++m`,
`net-http_headers.c++m`, `net-sse.c++m`, `net-mcp.c++m`, `net-websocket.c++m`, and
`net-websocket_frame.c++m`. TLS is external and out of scope.

Probes ran against the real modules through a temporary test in `tools/CB.sh debug test`
(removed afterwards). They reproduced #3, #4, and the header cases in #5. #2 was measured
with GCC 14 / libstdc++ (`gcc:14`, `alpine:3.20`) and Clang 21 / libc++ (the dev
container). Everything else is traced from code.

## Summary

| # | Severity | Finding |
|---|---|---|
| 1 | High | Connection-slot exhaustion: Slowloris, idle SSE/WS, slow reads |
| 2 | Low | `std::regex` route matching: recompiled per request; crashes only on libstdc++ builds |
| 3 | Medium | SSE field injection via bare CR, and LF in `event` / `id` / comments |
| 4 | Medium | MCP reflects the raw JSON-RPC `id` token into replies |
| 5 | Medium | Header parser leniency enables request smuggling |
| 6 | Medium | Session IDs / UUIDs have at most 32 bits of seed entropy |
| 7 | Medium | `select()` with `fd >= FD_SETSIZE` writes out of bounds |
| 8 | Medium | Request bodies and session IDs logged by default; log escaping is incomplete |
| 9 | Medium | WebSocket and plain SSE have no Origin / Host check by default |
| 10 | Medium | `listen(port)` binds 0.0.0.0, and MCP has no auth by default |
| 11 | Medium | Rate limiter trusts `X-Forwarded-For`; keys are unbounded |
| 12+ | Low / Info | HTTP client limits, metrics label injection, and others |

Suggested fix order: #1, #3, #4, #5, #6, #7. Fixing #3, #4, and #6 together closes
the cross-session MCP injection chain.

## High

### 1. Connection-slot exhaustion

- **Location:** `net-http_server.c++m`: `default_read_timeout`, `default_max_connections`,
  `handle()`. WebSocket and SSE call `read_timeout(0)` on takeover.
  `net-endpoint_buf.c++m` `send_all`.
- **CWE:** CWE-400, CWE-770.
- **Issue:** One thread per connection, and a global cap of 128 with no per-client limit.
  Three ways to hold a slot:
  - **Slowloris.** The 30 s timeout is the gap between bytes, not a deadline for the
    request. One byte every 29 s holds a slot for about 16 KiB × 29 s, roughly 5.5 days.
  - **Idle upgraded streams.** WebSocket and SSE set `read_timeout(0)`.
    `run_text_session` blocks forever, with no idle timeout and no server ping. MCP's
    1 s heartbeat only detects dead peers, not idle live ones.
  - **Slow reads.** Writes have no timeout. A client that never reads a large response
    blocks `send()` forever.
- **Exploitability:** 128 TCP connections from one host cause a complete outage. Every
  new client gets `503`.
- **Fix:** A total deadline for the request head and body. A write timeout. An idle
  timeout with ping / pong for WebSocket. A separate cap for streaming connections so they
  cannot starve normal requests. Optionally a per-client connection cap.
- **Owner:** Library.

## Medium

### 3. SSE field injection

- **Location:** `net-sse.c++m` `format_event`, `append_field`, `format_comment`.
- **CWE:** CWE-93, CWE-74.
- **Issue:** `data` is split only on `\n`. The SSE spec also ends lines on a bare `\r`,
  which passes through. `event`, `id`, and comment text are not checked; the comment on
  `append_field` says "must not contain CR/LF" but nothing enforces it. Probe:

  ```text
  format_event("hello\r\revent: endpoint\rdata: http://evil/\r\r", "message")
  → event: message<LF>data: hello<CR><CR>event: endpoint<CR>data: http://evil/<CR><CR><LF><LF>
  ```

  A browser `EventSource` or MCP client parses this as a separate, attacker-chosen
  `endpoint` event.
- **Fix:** Split `data` on `\r\n`, `\r`, and `\n`. Reject or strip CR, LF, and NUL in
  `event`, `id`, and comments.
- **Owner:** Library. Callers reasonably assume `format_event` makes data safe.

### 4. MCP reflects the raw JSON-RPC `id`

- **Location:** `net-mcp.c++m` `detail::json_id_field`, and the reply builders in
  `handle_json_rpc` that concatenate `*id`.
- **CWE:** CWE-116, CWE-74.
- **Issue:** The `id` is copied verbatim. Strings are not checked for raw control bytes, and
  non-string tokens are not type-checked. Probes:
  - A raw CR inside a string id survives into the SSE frame. Combined with #3 this is
    event injection, e.g. a forged `endpoint` event that redirects later POSTs:

    ```text
    data: {"jsonrpc":"2.0","id":"x<CR><CR>event: endpoint<CR>data: http://evil/messages/<CR><CR>","result":{}}
    ```

  - `{"id":{"result":{"forged":true},"method":"ping"}` yields
    `{"jsonrpc":"2.0","id":{"result":{"forged":true,"error":{…}}`.
  - The scanner accepts unterminated JSON (`{"id":true,"method":"ping"`) and non-spec id
    types (`true`).
- **Exploitability:** Posting to a session needs only its `session_id`. A client injecting
  into its own stream gains nothing; the risk is a third party who learns the id (#6, #8).
- **Fix:** Accept only a JSON number, `null`, or a string without unescaped control
  characters. Otherwise reply with id `null` and `-32600`. Reject messages that do not
  return to depth 0.
- **Owner:** Library.

### 5. Header parser leniency (smuggling behind a proxy)

- **Location:** `net-http_headers.c++m` `operator>>(std::istream&, headers&)`.
- **CWE:** CWE-444.
- **Issue:** Probes:
  - `Content-Length : 5` is accepted. RFC 9112 §5.1 says reject with `400`.
  - `\vContent-Length: 5` and `Content-Length\v: 5` are accepted. The obs-fold check only
    looks for SP / HTAB, and `utils::trim` strips `\f` and `\v` too.
  - `X: a\rb` keeps the bare CR. RFC 9112 §2.2 says reject or replace it.
  - The loop stops at any line starting with `\r` (`is.peek() != '\r'`), while
    `read_request_head` keeps reading. In
    `Host: a\r\n\r\r\nTransfer-Encoding: chunked\r\n\r\n` the `Transfer-Encoding` line is
    dropped, bypassing the TE rejection (traced, not probed).
- **Exploitability:** Needs a front-end proxy that forwards the malformed header without
  normalizing it. The proxy and this server then disagree on body length.
- **Fix:** Validate field names as RFC 9110 `token`, with no whitespace before `:`. Allow
  only SP / HTAB as optional whitespace. Reject CR, NUL, and other controls (except HTAB) in
  values. Treat a line that is not exactly `\r\n` or `\n` as malformed, not as end of headers.
- **Owner:** Library.

### 6. Session IDs and UUIDs have at most 32 bits of seed entropy

- **Location:** `net-uuid.c++m` (`thread_local std::mt19937_64 gen{std::random_device{}()}`),
  used by `mcp::make_session_id`. `net-websocket.c++m` `detail::random_u32` is seeded the
  same way.
- **CWE:** CWE-338, CWE-331.
- **Issue:** `random_device{}()` yields one 32-bit value. Each connection runs on a new
  thread, so each session ID is the first output of a freshly seeded Mersenne Twister: at
  most 2³² possible IDs, not 122 bits. Mersenne Twister is not a CSPRNG either.
- **Exploitability:** The `session_id` is the only thing tying a POST to an SSE stream and
  is the entry point for #4. It is not bound to the identity that opened the stream, so with
  `authorize()` any valid credential can post to another user's session.
- **Fix:** 16 bytes from `getentropy()` / `arc4random_buf()` (or `/dev/urandom`) for session
  IDs, and the same for WebSocket masking keys. Optionally bind the session to the
  authenticated principal.
- **Owner:** Library.

### 7. `select()` with `fd >= FD_SETSIZE` writes out of bounds

- **Location:** `net-endpoint_buf.c++m` `wait_readable` (runs on every read, since
  `read_timeout` defaults to 30 s), `net-socket.c++m` `wait_for` / `wait`,
  `net-acceptor.c++m` `wait`, `net-posix.c++m` `connect_with_timeout`.
- **CWE:** CWE-787.
- **Issue:** `FD_SET(fd)` for `fd >= 1024` writes past a stack `fd_set`. On Linux that is
  stack corruption, or an abort under `_FORTIFY_SOURCE`. On macOS the bit is silently not
  set and the call looks like a timeout.
- **Exploitability:** Needs more than 1024 open descriptors. Unlikely with the default cap,
  realistic in a host app (database files, `max_connections(0)`) with a raised
  `RLIMIT_NOFILE`, which is common in containers.
- **Fix:** Replace `select` with `poll()`.
- **Owner:** Library.

### 8. Sensitive data in logs; incomplete log escaping

- **Location:** default level `debug` in `net-structured_log_stream.c++m`
  (`process_defaults`). `HTTP_REQUEST_BODY` logs the full body. `HTTP_REQUEST` and others
  log the full URI, including `?session_id=`. `json_escape` and `sd_escape` in the log
  stream.
- **CWE:** CWE-532, CWE-117.
- **Issue:** Passwords, tokens, and MCP tool arguments in bodies are logged by default.
  MCP session IDs are logged on every POST. `json_escape` leaves most controls below 0x20
  unescaped (ESC, NUL, VT), producing invalid JSONL and allowing terminal-escape injection.
  In syslog mode `sd_escape` does not escape CR / LF, and the human message is raw.
- **Fix:** Default to `info`; log body size, not content. Redact the query string or at
  least `session_id`. Escape every control below 0x20 as `\u00XX`; escape CR / LF in syslog.
- **Owner:** Library.

### 9. WebSocket and plain SSE have no Origin / Host check by default

- **Location:** `net-http_server.c++m` WebSocket upgrade path (gate absent by default);
  `controller::gate()` is optional for SSE.
- **CWE:** CWE-1385 (Cross-Site WebSocket Hijacking), CWE-350.
- **Issue:** Any web page can open `ws://localhost:PORT/ws` with the victim's cookies and
  read the replies. No Host allowlist, so DNS rebinding also works against WebSocket, SSE,
  and ordinary routes. Only `http::mcp::server` ships Host / Origin defaults. The handshake
  also skips `Sec-WebSocket-Version: 13` and key-format validation (Low).
- **Fix:** Require an Origin allowlist for `ws()`, or reject a present-but-unlisted Origin
  by default. Expose the MCP `value_allowed` Host / Origin gate as reusable middleware.
- **Owner:** Caller today; the library should default to safe.

### 10. `listen(port)` binds all interfaces, and MCP has no auth by default

- **Location:** `http::server::listen(std::string_view)` calls `listen("0.0.0.0", …)`.
  `mcp::server::authorize` is optional.
- **CWE:** CWE-1188.
- **Issue:** DNS-rebinding protection is a browser defence, not authentication. A
  non-browser client on the LAN sends `Host: localhost:8080` and can call every tool. The
  README examples use `127.0.0.1`, but the single-argument overload exposes the server on
  all interfaces. `is_wildcard_bind_host()` exists but is unused.
- **Fix:** Default the single-argument `listen` to loopback. In `mcp::server::attach`, warn
  or refuse on a wildcard bind without `authorize()`.
- **Owner:** Library defaults.

### 11. Rate limiter is spoofable and grows without bound

- **Location:** `net-http_server_middlewares.c++m` `default_rate_limit_key_extractor`,
  `rate_limiter::check_limit`.
- **CWE:** CWE-290, CWE-770.
- **Issue:** The key is the raw `X-Forwarded-For`. Rotating it bypasses the limit; setting
  it to a victim's IP locks the victim out. Without the header every client shares one
  `"unknown"` bucket. Unique keys of up to about 16 KiB each are kept for the window, and
  `flat_map` insertion is O(n): memory and CPU exhaustion.
- **Fix:** Default to the socket peer address. Trust `X-Forwarded-For` only from configured
  proxies, right-most untrusted hop. Cap key count and length; use a hash map or LRU.
- **Owner:** Library default; callers can pass their own extractor. See also item 5 in
  [http-server-scan-improvements.md](http-server-scan-improvements.md), which covers the
  proxy-overwrites-header deployment.

## Low / Informational

### 2. Route regex matching

Originally rated High from a GCC test harness. Downgraded after measuring with the
toolchain `CB.sh` always uses (`-stdlib=libc++`).

- **Location:** `net-http_server.c++m`, the three `std::regex_match(…, std::regex{path})`
  calls in `handle()` (WebSocket, SSE, and normal routing).
- **CWE:** CWE-674, CWE-1333.
- **Issue:** libstdc++'s regex engine recurses once per character for repeated groups.
  libc++ does not. Pattern `/api/(a|b)*` against `/api/aaa…`, matched in a worker thread:

  | Build | Thread stack | Crashes at |
  |---|---|---|
  | GCC 14 / libstdc++, glibc | 8 MiB | between 8 KiB and 12 KiB of URI |
  | GCC 14 / libstdc++, musl (Alpine) | 128 KiB | 256 characters |
  | Clang 21 / libc++ | 128 KiB or 8 MiB | no crash at 64 KiB |

  With libc++, catastrophic backtracking (`/api/(a+)+` against 16+ `a`s and a mismatch)
  ends quickly with `regex_error` (error_complexity). `handle()` catches it, answers `500`,
  and closes the connection; the server keeps running. MCP's own route
  (`/messages/?(\?.*)?`) is safe on both libraries.

  What remains on the supported toolchain: every route regex is recompiled on every
  request, and an invalid route pattern surfaces as a `500` per request instead of at
  registration.
- **Fix:** Compile route regexes once at registration. Optionally cap the request-target
  length (`414`). If libstdc++ builds are ever supported, replace `std::regex` with a
  non-recursive or segment-based matcher; a length cap does not help on musl.
- **Owner:** Library for the recompilation; the patterns themselves are app-supplied.

- **HTTP client** (`net-http_client.c++m`): `method`, `target`, and header values are not
  checked for CR / LF (request splitting, CWE-113). Response headers and body are unbounded
  (`body.resize(Content-Length)` or read to EOF). No read timeout after connect. Chunked
  responses are not decoded. Low to Medium depending on whether the client talks to
  untrusted servers.
- **Prometheus label injection** (`metrics_registry::render_prometheus_text`): the `path`
  label comes from the request target and is not escaped. A `"` breaks out of the label and
  Prometheus rejects the whole scrape. `/metrics` is served unauthenticated by any route the
  middleware wraps. CWE-116.
- **`authentication_middleware` passes path and query to `is_public_path`:** predicates such
  as `ends_with(".css")` are bypassed by `/admin?x=.css`. Document it, or pass only the path.
- **Route precedence is lexicographic** (`flat_map` router): `/.*` sorts before `/admin`, so
  a broad public route can shadow a protected one.
- **OPTIONS and HEAD fall back to the GET handler:** without `cors_middleware`, an OPTIONS
  request runs the GET handler's side effects.
- **MCP error message leak:** a tool's `e.what()` is returned to the client. CWE-209.
- **MCP scanner versus a strict parser:** accepts trailing commas and unterminated objects.
  A body-aware `authorize()` must fail closed on parse errors. Without `list_tools`, any tool
  name reaches `call_tool`.
- **MCP Content-Type check** runs only when the header is present. Matters only with
  DNS-rebinding protection disabled (header-less no-cors POST).
- **WebSocket close frame:** the server echoes the peer's close payload without validating
  length 1, reserved codes (1005 / 1006 / 1015), or UTF-8 reasons (RFC 6455 §7.4). The client
  masking key is not from a strong entropy source (§5.3).
- **`url_decode`** (`net-uri.c++m`): decodes `%00` and `%2F`, turns `+` into a space
  everywhere, and `from_chars` accepts `-` in the hex pair. Re-validate decoded file paths.
- **`url_encoded`** (`net-http_escape.c++m`): locale-dependent `std::isalnum`, so non-ASCII
  bytes can be emitted raw under non-C locales. Leaves `setfill('0')` on the caller's stream.
- **Socket hygiene** (`net-acceptor.c++m`): no `SOCK_CLOEXEC` / `accept4`, listen backlog 5,
  and a theoretical `close()` versus `accept()` race on a reused fd.

## Security-relevant defaults

Safe:

- 16 KiB request-head cap (`431`).
- 1 MiB body cap, checked before allocation.
- 30 s idle read gap.
- 128-connection cap.
- 1 MiB WebSocket frame cap.
- MCP inbound queue capped at 256 messages / 1 MiB.
- MCP Host / Origin allowlist defaulting to loopback.
- `X-Frame-Options: DENY`, `nosniff`, `Referrer-Policy: no-referrer`, `Permissions-Policy`.
- Fixed `Server: net`.

Unsafe:

- Debug-level logging of request bodies.
- `listen(port)` binding to 0.0.0.0.
- No authentication on MCP.
- No Origin or Host check on WebSocket, plain SSE, or ordinary routes.
- Rate limiter keyed on `X-Forwarded-For`.
- CORS `allow_credentials = true`.

## Where the library relies on the caller

- HTML and URL escaping of handler output (`html_escaped` / `url_encoded` are opt-in).
- Origin / Host gates for WebSocket, SSE, and ordinary routes.
- Validating MCP tool arguments; `arguments` is passed through as raw JSON.
- Making `authorize()` fail closed.
- Choosing route regexes that are safe on libstdc++.
- Sanitising SSE `event` / `id` values (until #3 is fixed).
- Validating paths after `url_decode`.

## Positive properties

- **Request framing:** `Transfer-Encoding` rejected outright. Duplicate `Content-Length` and
  `Host` rejected. `Content-Length` parsed strictly with `from_chars`. obs-fold rejected.
  Missing `Host` is `400`. Unsupported `Expect` is `417`.
- **Response headers:** framing / hop-by-hop names and CR / LF / NUL are filtered from
  custom headers, status lines, and content types. Reflected `Origin` is CR-checked.
- **`X-HTTP-Method`:** client values are overwritten by the server.
- **WebSocket:** client frames must be masked. RSV bits, opcodes, and control-frame rules are
  enforced. 64-bit lengths are checked against the cap before allocation. The UTF-8 validator
  is strict (overlongs, surrogates, > U+10FFFF). Fragmented and binary frames fail closed.
  `Upgrade` / `Connection` are matched as exact tokens.
- **MCP:** the key scanner rejects duplicate keys, compares keys after unescaping, rejects
  leading primitives and concatenated values, and fails closed on non-ASCII `\u` escapes.
  Queue sizes and byte counts have overflow checks.
- **Other:** SIGPIPE handled with `MSG_NOSIGNAL` / `SO_NOSIGPIPE`. Streams use the classic
  locale. Accept-time EMFILE / ENOMEM is retried rather than ending the listen loop.
