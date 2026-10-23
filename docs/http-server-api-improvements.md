# HTTP server API: remaining improvements

**Date:** 23 October 2026
**Repo:** [ruoka/net4cpp](https://github.com/ruoka/net4cpp)
**Status:** Open items from the October 2026 review of the public `http::server` C++ API.
The fixes already made are listed at the end for context.

Consumers checked against these changes: YarDB, fixer (dashboard), and 3xS (which
builds both through their nested `deps/net`).

## Consumer migrations

These keep compiling today; deprecated names only warn.

| Consumer | `.response_with_headers(` | `callback_with_headers` | `response_with_headers{...}` |
|----------|---------------------------|-------------------------|------------------------------|
| YarDB    | 16 | 1 | 63 |
| fixer    | 7  | 0 | 0  |
| 3xS      | 87 | 0 | 5  |

- `.response_with_headers(ct, cb)` becomes `.response(ct, cb)`, and
  `http::callback_with_headers` becomes `http::callback`. Both are mechanical renames.
- `response_with_headers{status, content, headers}` becomes
  `http::make_response(status, content, headers)`. Brace construction is outside the
  documented contract for the response type (see `docs/release-policy.md`).
- YarDB also has 19 `std::get` / `std::tie` calls and fixer has 1. Any that read a
  handler response should move to a structured binding.

## Response type representation

`http::response_with_headers` is a `std::tuple<status, content, std::optional<headers>>`.
It is exported so code can name it, and its contract is: create with `make_response` /
`make_error_response`, read with `auto [status, content, headers] = ...`.

Open:

- Replace the tuple with a named aggregate (`status`, `content`, `headers` members, same
  order). Brace construction and structured bindings keep compiling; `std::get` and
  `std::tie` do not, so do this after the consumer migrations above.
- Status is a `std::string` such as `"200 OK"`. A status type would catch typos at
  compile time; it changes every handler, so it needs its own design.

## Request handlers

`http::request` is available to normal handlers. Follow-ups:

- SSE and WebSocket handlers and the takeover gate still receive `request_view` only.
- The server still writes `x-http-method` into the request headers for old-style
  handlers, next to client-supplied headers. Remove it with the old interface.
- Deprecate the old `(request_view, body_view, headers&)` handler shape,
  `http::callback`, `http::middleware_factory` and the matching `wrap` overload once
  YarDB, fixer and 3xS have moved to `const http::request&`. Remove in the next major.
  `http::request_view` itself stays; it is used outside handlers.
- `net-readme_examples.test.c++` does not compile the README "Request handlers" example.

## Middleware

- Most built-in middlewares take the request headers by value (`auto hdr`); only
  `correlation_id_middleware` takes them by reference. A header set by an inner
  middleware may not reach outer middlewares or the server. Unverified: write a test
  before deciding whether it is a bug.
- There is no server-level `use(...)`; each route wraps its own chain with
  `middleware::wrap`. A `use` needs a decision on whether it applies at registration
  or at dispatch, and to which routes.

## Naming

- `destroy()` registers DELETE handlers. Kept: `delete` is a keyword, and a second name
  would reintroduce the duplicate registration methods that were just deprecated.

## Documentation

- Each accepted connection runs on its own thread, bounded by `max_connections`.
  The README does not state this as a scaling limit.

## Done in this round

- Router keeps `controller&` stable (node-based storage), compiles each route regex
  once at registration, and matches in registration order (first match wins).
- A route regex that gives up (`regex_error`, error_complexity in libc++) counts as no
  match: later routes are still tried and an unmatched target gets 404. The README says
  how to write route patterns that reject cheaply.
- `response_handler`, `response_with_headers`, `response_with_headers_handler` and
  `callback_with_headers` deprecated in favour of `response` and `callback`.
- `http::request` (method, target, peer, route; `path()`, `query()`, `query(key)`,
  `capture(n)` on demand), accepted by `controller::response()` next to old handlers.
  Built-in middlewares wrap both; `request_middleware_factory` and a `wrap` overload.
- `text()` … `xml()` return `controller&`; server setters return `server&`.
- `takeover_gate` / `gate_handler()` replace `sse_gate` / `sse_gate_fn()` (deprecated).
- A route registered without a handler answers 404, not 200 `"Not Found"`.
- `docs/release-policy.md`: response type contract and the deprecation rule.
