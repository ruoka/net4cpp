# HTTP server scan improvements

**Branch:** `docs/http-server-scan-improvements`  
**Repo:** [ruoka/net4cpp](https://github.com/ruoka/net4cpp)  
**Status:** Items 1 through 4 and 6 through 10 are implemented. Item 2 includes a 30s idle read timeout, `Expect: 100-continue`, and a 128-connection cap. Item 5 is not a code change when a TLS proxy overwrites `X-Forwarded-For` and the origin port is private. `Content-Security-Policy` and `Strict-Transport-Security` stay off this server. Item 11 stays on the proxy.  
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

Implemented. `http::server` sets `endpointstream::read_timeout` from `read_timeout()` (default 30s, separate from the accept poll). A partial head or body gets `408`. An idle keep-alive with no bytes is closed without a response. WebSocket and SSE clear the timeout when they take the socket. `Expect: 100-continue` gets `100 Continue` before the body is read, when that body will be accepted. Any other expectation is `417` and the body is not read.

Accepted connections are capped by `max_connections()` (default `default_max_connections`, 128). Zero disables the cap. A connection past the cap gets `503` with `Retry-After: 1` on the accept thread and does not start another handler. `server::timeout` still only bounds the accept `select`.

Public Slowloris probes usually die at the proxy idle timeout. The cap is what stops the same probe against the origin port from spawning a thread per connection.

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

Not a code change for the usual deployment. `rate_limiting_middleware` is opt-in, and `default_rate_limit_key_extractor` is only the key when the app uses it. A TLS proxy that overwrites `X-Forwarded-For` or `X-Real-IP` is the thing the origin sees, so the key is the client address the proxy chose. The origin port stays private, so a client cannot open a socket and supply its own header.

It is a real bypass only when the origin port is reachable. An appending proxy used to be a second bypass (each spoofed prefix was a distinct key); `default_rate_limit_key_extractor` now keys on the rightmost `X-Forwarded-For` entry, which only the proxy wrote. Omitting the header puts those requests in one `"unknown"` bucket, which is strict rather than a bypass, and a proxy that always sets the header does not hit that path. The key map can still grow with distinct clients; that is ordinary limiter state, not something a public scan reaches if the proxy is doing its job.

Leave the extractor as it is. The proxy checklist still says to overwrite the forwarding headers.

## 6. Check `Origin` on WebSocket upgrade

Implemented with the same optional `controller::gate()` SSE already uses. It runs after the route and `Sec-WebSocket-Key` check and before `101`. No gate means a missing `Origin` still upgrades, so non-browser clients keep working. A browser-facing route installs a gate that returns `403` when `Origin` is missing or not on the allowlist.

`is_websocket_upgrade` still only looks at `Upgrade`, `Connection`, and the key. The gate is what applies the origin rule. A page on another host can otherwise open a cross-site WebSocket straight to the origin port. The TLS proxy does not cover that raw port.

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
- Idle timeout, a body-size cap, and a connection cap in front of the origin. The server now has all three as well.
- `Server` is the fixed token `net`. Replace it at the proxy only if a scan still wants it gone.
- Overwrite `X-Forwarded-For` / `X-Real-IP` rather than appending, if the app uses `rate_limiting_middleware`.
- Keep the origin port off the public internet. Cleartext listen defaults to `0.0.0.0`.
