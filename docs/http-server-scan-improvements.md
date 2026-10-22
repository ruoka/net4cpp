# HTTP server scan improvements

**Branch:** `docs/http-server-scan-improvements`  
**Repo:** [ruoka/net4cpp](https://github.com/ruoka/net4cpp)  
**Status:** Items 1, 3, 4, 7, 8, 9, and 10 are implemented. Item 2 applies a 30s idle read timeout and answers `Expect: 100-continue`. Still open: a connection cap, rate-limit keys (item 5), and WebSocket `Origin` (item 6). `Content-Security-Policy` and `Strict-Transport-Security` stay off this server. Item 11 stays on the proxy.  
**Date:** 2026-10-22

## Scope

Priority order for `http::server` after comparing it with a typical vulnerability scan
(Nikto, ZAP, Qualys, Burp, securityheaders.com). TLS is terminated by a cloud proxy.
Cipher suites, certificates, protocol versions, and the HTTP-to-HTTPS redirect stay on
that proxy. This list is what the scan still reports, or what the proxy does not cover
if the origin port is reachable.

Code: `net/net-http_server.c++m`, `net/net-http_headers.c++m`,
`net/net-http_server_middlewares.c++m`, `net/net-websocket.c++m`.

Leave these alone. A scan already passes them:

- TRACE / TRACK are not implemented. Unknown methods get `400` and the request is not echoed.
- `CONNECT` and absolute-form request targets are not proxied.
- No static files, so path traversal and directory listing do not apply.
- Any `Transfer-Encoding` is rejected before the body is read.
- Duplicate `Content-Length` and duplicate `Host` are rejected. A missing `Host` is `400`.
- Custom response headers drop CR, LF, and NUL, and cannot override framing or hop-by-hop names.
- `500` bodies are empty. Exception text stays in the log.
- The request head is capped at 16 KiB (`431`).
- `cors_middleware` does not fall back to `Access-Control-Allow-Origin: *`.

## 1. Cap the request body before allocation

Implemented. The default is `default_max_request_body_size` (1 MiB), still overridable, and the existing check rejects the request before the body is allocated.

`max_request_body_size` used to default to `std::numeric_limits<std::size_t>::max()`. The
server allocates `std::string(content_length)` before the handler runs.
`body_size_validation_middleware` sees the body only after that allocation, so it
cannot stop it. A `Content-Length` near `2^63` becomes `bad_alloc` and a `500`.

A cloud proxy often has its own body cap, which is why a public scan may miss this.
The `413` response was already in place; the default is what changed.

## 2. Time out and cap accepted connections

The read deadline is in place. `http::server` sets `endpointstream::read_timeout` from `read_timeout()` (default 30s, separate from the accept poll). A partial head or body gets `408`. An idle keep-alive with no bytes is closed without a response. WebSocket and SSE clear the timeout when they take the socket. `Expect: 100-continue` gets `100 Continue` before the body is read, when that body will be accepted. Any other expectation is `417` and the body is not read. There is still no connection cap.

`server::timeout` only bounds the accept `select`. Each connection is still its own thread, with no cap.

Public Slowloris probes usually die at the proxy idle timeout. The same probe against
the origin port hangs one thread per connection.

Still open: a maximum number of in-flight connections. The read deadline and
`Expect` handling are done.

## 3. Stop putting the machine name in `Server`

Implemented. Responses send `Server: net` (`server_token`).

Every response used to send `Server: ` plus `gethostname()`. Scanners report that as internal
hostname disclosure. It shows up on the public URL unless the proxy replaces the
header.

Send a fixed token (`net`) or omit `Server`. Do not include the hostname or a version.

## 4. Default browser headers, and a real error `Content-Type`

Implemented, except `Content-Security-Policy`. Every response sends `X-Content-Type-Options: nosniff`, `Referrer-Policy: no-referrer`, `X-Frame-Options: DENY`, and `Permissions-Policy: camera=(), microphone=(), geolocation=()`. Protocol errors and `404`s use `text/plain; charset=utf-8`. `frame_options(false)` omits `X-Frame-Options` so an embedded page can set its own. A custom `Referrer-Policy` or `Permissions-Policy` does not replace the server values.

`Content-Security-Policy` stays opt-in. A default of `default-src 'none'` breaks
`.html()` pages that load scripts. `Strict-Transport-Security` stays on the proxy:
browsers ignore it on the cleartext origin hop, and the proxy is what the browser
talks to.

## 5. Rate-limit on a trusted client address

`default_rate_limit_key_extractor` keys on the raw `X-Forwarded-For` or `X-Real-IP`
value. It never sees the TCP peer. Rotating the header skips the bucket. Omitting it
puts every such client in `"unknown"`. A proxy that appends the real address still
leaves the first value client-controlled unless it overwrites the header. Distinct
values also accumulate in the limiter map; cleanup only drops empty keys.

Pass the accepted peer address into the middleware. Trust forwarding headers only
when the app opts in and says how many proxy hops to skip (use the last untrusted
value, not the whole header). Cap the number of keys.

## 6. Check `Origin` on WebSocket upgrade

`is_websocket_upgrade` accepts the connection from `Upgrade` and `Connection` plus
`Sec-WebSocket-Key`. There is no `Origin` check. SSE already has an optional `gate()`
for Host and Origin. WebSocket does not.

A browser can open a cross-site WebSocket, and a page on another host can reach the
origin port directly (DNS rebinding). The TLS proxy does not cover that raw port.

Apply the same kind of gate used for SSE before `101 Switching Protocols`, and reject
the upgrade when `Origin` is missing or not allowed.

## 7. Send `Vary: Origin` when CORS reflects an origin

Implemented. `cors_middleware` appends `Origin` to `Vary` when it reflects an origin, and the SSE response head does the same.

`cors_middleware` reflects an allowlisted `Origin` and defaults
`Access-Control-Allow-Credentials` to true. A caching proxy can store
`Access-Control-Allow-Origin: https://a` and reuse it for another site when `Vary`
is missing. `Cache-Control: private` on normal responses limits this for shared caches.

Credentials stay on only with a concrete allowlisted origin. A predicate which
returns true for every origin, or for `null`, reopens the finding.

## 8. Reject obsolete line folding

Implemented. A header line that starts with a space or tab, has no colon, or has an empty name throws. `http::server` already turns that into `400` and closes.

Header lines with no colon used to be skipped. A folded continuation (`Content-Length: 5`
then a line ` 0`) was ignored rather than rejected. RFC 9112 says a server rejects
obs-fold or unfolds it.

## 9. Filter the status line and `Content-Type` the same way as custom headers

Implemented. CR, LF, or NUL in the handler status or content type becomes `500` with the stock status text and `text/plain; charset=utf-8`. The injected line is not written. The same check applies to an SSE gate denial status.

`write_custom_response_headers` already dropped CR, LF, and NUL. The status line and
`Content-Type` used to be written directly from the handler result.

## 10. Basic auth: CSRF and realm quoting

Implemented as far as this library goes. `"` and `\` in the realm are escaped, and CR, LF, and NUL are omitted, before `WWW-Authenticate` is built. The middleware comment states that browsers resend `Authorization` on cross-site requests and that this middleware does not issue a CSRF token. State-changing routes need that defense outside it.

Only when `authentication_middleware` is used. Browsers resend `Authorization` on
cross-site requests, so state-changing routes show up as CSRF.

## 11. Proxy checklist

Not code in this library. A public scan fails these when the proxy is left at defaults:

- `Strict-Transport-Security` on the HTTPS response.
- HTTP on port 80 redirects to HTTPS. This server does not redirect.
- Idle timeout and a body-size cap in front of the origin. The server now has both, and still has no connection cap.
- `Server` is the fixed token `net`. Replace it at the proxy only if a scan still wants it gone.
- Overwrite `X-Forwarded-For` / `X-Real-IP` rather than appending, until item 5 exists.
- Keep the origin port off the public internet. Cleartext listen defaults to `0.0.0.0`.
