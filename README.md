# net4cpp
Network library based on **C++23 modules**.

## Requirements

- **Clang 21+** on Linux (`clang++-21`; CB and CI default to LLVM 21)
- **macOS**: locally built LLVM at `/usr/local/llvm` (see `deps/tester` README)
- Open in a dev container (`.devcontainer/`) for a reproducible Linux toolchain

This subproject is designed to work both:
- **Standalone** (as its own repo, with `deps/tester` available), and
- **As a dependency inside YarDB** (reusing YarDB’s `deps/tester`).

## Versioning & Releases

**Current release: [`v3.4.1`](https://github.com/ruoka/net4cpp/releases/tag/v3.4.1)** — SemVer tag for the Clang 21 + libc++ modules consume surface. Prior `v1.0` / `v2.0` / `v2.1` tags are historical.

See [`docs/release-policy.md`](docs/release-policy.md) for the public surface, SemVer rules, and release criteria. Nested `deps/tester` must match sibling libraries and YarDB — [ecosystem versioning](https://github.com/ruoka/YarDB/blob/master/docs/versioning.md).

## Build

Build and run tests with the project’s C++ Builder wrapper:

```bash
cd deps/net
tools/CB.sh debug test
```

### Network tests

By default, **network tests run** (they may require local privileges, free ports, multicast enabled, etc.).

To disable network/integration tests:

```bash
NET_DISABLE_NETWORK_TESTS=1 tools/CB.sh debug test
```

Notes:
- In Cursor sandbox environments, `tools/CB.sh` auto-sets `NET_DISABLE_NETWORK_TESTS=1` unless you override it.
- Multicast tests depend on the host/network allowing multicast.

## Echo server

```cpp
import net;
import std;

try
{
    using namespace std::string_literals;
    using namespace net;

    auto ator = acceptor{"::1", "2112"}; // IPv6 localhost
    auto [stream,client,port] = ator.accept();

    while(stream)
    {
        auto echo = ""s;
        getline(stream, echo);
        stream << echo << endl;
        clog << echo << endl;
    }
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}
```

## HTTP server

`http::server` maps a request path to a response body. `get("/").text(...)` answers
`GET /` with status `200` and `text/plain`, and `listen` accepts connections on that
address. TLS is not built in; terminate it in a proxy in front of the server.

```cpp
import net;
import std;

try
{
    auto server = http::server{};
    server.get("/").text("Hello, World!");
    server.get("/api/status").json(R"({"status": "ok"})");
    server.listen("127.0.0.1", "8080");
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}
```

### Routes

Route paths are regular expressions, compiled once at registration and matched
against the whole request target, query string included: `/orders/[0-9]+` does not
match `/orders/7?sort=asc`, and `/orders/[0-9]+(\?.*)?` does. WebSocket and SSE routes
match the path only. Routes are tried in registration order; the first
route that matches and has a handler for the method answers, and routes do not chain.
A catch-all such as `get(".*")` registered last is a fallback, not a filter: it only
sees requests no earlier route answered. Use [middlewares](#middlewares) for code that
runs on every route.

The target is client-controlled, so keep route patterns cheap to reject:

- Start with a literal prefix (`/orders/[0-9]+`, not `/.*orders`), so unrelated
  targets fail on the first characters.
- Avoid nested or overlapping repetition such as `(\w+/?)+` or `(a|a)*`. A near-miss
  target makes `std::regex` try every way to split it. libc++ gives up after a fixed
  amount of work; the server then logs `HTTP_ROUTE_REGEX_ERROR`, treats the route as
  not matching, and goes on to the next one.

A literal prefix alone does not make a pattern safe: `/(\w+/?)+` starts with `/`
and still gives up on `/aaaaaaaaaaaaaa!`.

### Request handlers

A handler that spells out `const http::request&` gets the method, target, peer
address and matched route. Nothing is parsed until it asks: `path()`, `query()`,
`query(key)` and `capture(n)` split the target or rerun the route regex on demand.
`query(key)` returns the raw, still percent-encoded value; decode it with
`net::url_decode`. Handlers taking `http::request_view` (the raw target) keep working
unchanged.

Return a response with `http::make_response(status, content, headers)` or
`http::make_error_response(status, error, message)`, using the `http::status_*`
constants. Read one with a structured binding, `auto [status, content, headers] = ...`.
The response type is exported only so code can name it; brace construction,
`std::get` and `std::tie` are not part of the API.

```cpp
auto server = http::server{};
server.get(R"(/orders/([0-9]+)(\?.*)?)").response("application/json",
    [](const http::request& req, http::body_view, http::headers&) {
        const auto id = req.capture(1);
        const auto sort = req.query("sort").value_or("asc");
        return http::make_response(http::status_ok,
            std::format(R"({{"id":{},"sort":"{}","from":"{}"}})", id, sort, req.peer));
    });
server.listen("127.0.0.1", "8080");
```

WebSocket and SSE handlers and their gates take the request too; see
[Server-Sent Events](#server-sent-events) and [WebSocket](#websocket).

### Middlewares

A middleware wraps a handler and can answer early or pass the request on.
`http::middleware::wrap(handler, middlewares)` builds the chain for one route; the
first middleware in the list is the outermost and runs first. There is no
server-wide `use(...)`, so build the list once and wrap each route with it.

| Middleware | Does |
|------------|------|
| `authentication_middleware(is_public, validate)` | `401` unless the path is public or `Authorization` validates |
| `body_size_validation_middleware(max)` | `413` for larger bodies |
| `accept_validation_middleware(type)` | `406` when `Accept` excludes `type` |
| `correlation_id_middleware()` | Adds `x-correlation-id` to the request headers when missing |
| `rate_limiting_middleware(max, window, key)` | `429` past `max` requests per `window` and key |
| `cors_middleware(allowed_origin)` | Answers preflight and adds CORS headers for allowed origins |
| `metrics_middleware()` | Prometheus counters and latency; answers `GET /metrics` |

Middlewares take the request headers by reference, so a header one of them adds
reaches the ones inside it, the handler, and the ones outside it. Both handler shapes
work: wrap a `const http::request&` handler with `http::request_middleware_factory`
entries, or a `request_view` handler with `http::middleware_factory` entries.

```cpp
auto is_public = [](std::string_view target) { return target == "/health"; };
auto validate = [](std::string_view authorization) { return authorization == "Bearer secret"; };

const auto common = std::vector<http::request_middleware_factory>{
    http::middleware::correlation_id_middleware(),
    http::middleware::authentication_middleware(is_public, validate, "orders"),
    http::middleware::body_size_validation_middleware(64 * 1024),
};

auto order = [](const http::request& req, http::body_view, http::headers& hdr) {
    return http::make_response(http::status_ok,
        std::format(R"({{"id":"{}","correlation":"{}"}})", req.capture(1), hdr["x-correlation-id"]));
};

auto server = http::server{};
server.get("/orders/([0-9]+)").response("application/json", http::middleware::wrap(order, common));
server.get("/health").text("OK");
```

### Connections and threads

Each accepted connection runs on its own thread for as long as the client keeps it
open. `max_connections` (default 128) caps those threads; past the cap the accept
thread answers `503` itself. WebSocket and SSE sessions hold their thread for the
whole session, so `max_streaming_connections` (default 64) keeps them from taking
every slot. `max_connections_per_client` is off by default, because behind a reverse
proxy every connection comes from the proxy's address. This model suits internal
services and dashboards; for many thousands of concurrent clients, put a proxy in
front or use an event-driven server.

The read, request and write timeouts, the request head and body caps, and the
`Server` token have setters of the same name; every setter returns `server&`.

## HTTP client

`http::client::request` sends one HTTP/1.1 request on a new connection
(`Connection: close`) and reads the whole response into `status_code`, `status`,
`headers` and `body`. It sets `Host`, and defaults `Accept` and, when there is a body,
`Content-Type` to `application/json`; headers you pass win. Connect, transport and
parse failures throw. A non-2xx status is returned, not thrown: check `ok()`.

There is no TLS. A `net::uri` overload takes host and port from the URI (`https`
only changes the default port to 443), so reach HTTPS services through a proxy.

```cpp
import net;
import std;

auto res = http::client::request("127.0.0.1", "8080", "GET", "/api/status");
if(res.ok())
    std::clog << res.status_code << ' ' << res.body << '\n';

auto hdrs = http::headers{};
hdrs.set("authorization", "Bearer secret");
auto created = http::client::request("127.0.0.1", "8080", "POST", "/orders",
                                     hdrs, R"({"item":"book"})", std::chrono::seconds{5});
```

For a raw request, write to a `net::connect` stream directly:

```cpp
import net;
import std;

try
{
    using namespace net;

    auto s = connect("www.google.com", "http");

    s << "GET / HTTP/1.1"                << crlf
      << "Host: www.google.com"          << crlf
      << "Connection: close"             << crlf
      << "Accept: text/plain, text/html" << crlf
      << "Accept-Charset: utf-8"         << crlf
      << crlf
      << flush;

    while(s)
    {
        auto c = ' ';
        s >> noskipws >> c;
        clog << c;
    }
    clog << flush;
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}
```

## HTML and URL escaping

When building HTML responses by hand, escape untrusted data at the point of output.
`http::html_escaped` and `http::url_encoded` are stream adaptors — they write directly
into an `ostringstream` without per-field `std::string` temporaries.

```cpp
import net;
import std;

using namespace std::string_literals;

auto symbol = "NOKIA"s;
auto reference = R"(ACC+"<x>)"s;

auto contents = std::ostringstream{};
contents << "<td>" << http::html_escaped{symbol} << "</td>";
contents << "<a href=\"/cancel?ref=" << http::url_encoded{reference} << "\">Cancel</a>";

// Decode URL-encoded values with net::url_decode (net:uri module).
auto decoded = net::url_decode("ACC%2B%22%3Cx%3E");
```

Use `html_escaped` for text nodes and attribute values. Use `url_encoded` for query
parameter values in `href` and `action` URLs. The HTTP server does not escape
response bodies automatically.

## Server-Sent Events

SSE takes over a `GET` on `http::server`, parallel to WebSocket. Register with
`server.sse(path).sse(handler)`; a matching `GET` returns `200` with
`Content-Type: text/event-stream` (no `Content-Length`), then runs
`http::sse::session` until the handler returns or the write path fails.

The handler receives the session, the request and the request headers. Spell out
`const http::request&` to get method, peer and `capture(n)`; `auto` or
`http::request_view` gets the raw target. SSE and WebSocket routes match the path
without the query, and `capture(n)` follows that.

Middlewares do not wrap takeovers, and browsers send cookies on cross-site
`EventSource` requests without a preflight, so check `Origin` in a gate.
`http::middleware::origin_gate(origins)` rejects a missing or unlisted `Origin` with
`403`; install it only on routes browsers use. `.cors(allow)` reflects an allowed
`Origin` on the response head.

```cpp
import net;
import std;

auto server = http::server{};
auto allow = [](std::string_view o) { return o == "http://localhost:3000"; };

// Browser preflight (reuse cors_middleware).
server.options("/events/.*").response("text/plain",
    http::middleware::cors_middleware(allow)([](auto&&, auto&&, auto&&) {
        return http::make_response(http::status_no_content, "");
    }));

server.sse(R"(/events/(\w+))")
    .gate(http::middleware::origin_gate({"http://localhost:3000"}))
    .cors(allow)
    .sse([](http::sse::session& session, const http::request& req, http::headers& hdr) {
        if(hdr.contains("last-event-id"))
        {
            // resume policy is application-defined
        }
        session.send_comment("ok");
        session.send_event("topic", req.capture(1), "1");
    });
server.listen("127.0.0.1", "8080");
```

Notes:

- The session flushes after each event and comment. Proxies that buffer SSE may
  need `X-Accel-Buffering: no` (optional app header, not set by default).
- After the SSE stream ends, the connection is closed (no keep-alive reuse).

## WebSocket

Upgrade is handled inside `http::server`, not as response middleware. Register a path
with `server.ws(path).ws(handler)`; a matching `GET` with `Upgrade: websocket`
returns `101` and runs a text-frame session (ping/pong/close plus optional text
replies). The handler receives each text message, and optionally the request first;
return `net::websocket::text_reply` to answer, or `std::nullopt` for no reply.
`.gate(...)` works as for SSE.

Clients use `net::websocket::connect` for the same text session (masked outbound
frames, automatic ping/pong, `send` / `recv` / `read_loop` / `close`). Overloads take
`(host, port, path)` or a `net::uri` (`ws://host:port/path`). The connect `timeout`
bounds TCP connect and the full upgrade-response read (status line and headers).
`close()` waits briefly for the peer close frame (including truncated frames), then
force-closes (default 2s; RFC 6455 §7.1.1). No `wss://`; terminate TLS in front of
the server if needed.

Framing policy (fail closed with a close frame, no silent drops):

- Client-to-server frames must be masked (`1002` if not).
- Complete text frames only — `FIN=0`, `continuation`, and `binary` → `1003`.
- Text payloads must be valid UTF-8 (`1007`).
- Control frames must be FIN and ≤ 125 bytes; RSV bits must be 0 (`1002`).
- Payloads larger than 1 MiB → `1009` (configurable with `server::websocket_limits`).

```cpp
import net;
import std;

// Server
auto server = http::server{};
server.ws("/echo").ws([](std::string_view msg) {
    return net::websocket::text_reply{std::string{msg}};
});
server.ws(R"(/rooms/(\w+))")
    .gate(http::middleware::origin_gate({"http://localhost:3000"}))
    .ws([](const http::request& req, std::string_view msg) {
        return net::websocket::text_reply{std::format("{}: {}", req.capture(1), msg)};
    });
server.listen("127.0.0.1", "8080");

// Client
auto ws = net::websocket::connect("127.0.0.1", "8080", "/echo");
// or: auto ws = net::websocket::connect(net::uri{"ws://127.0.0.1:8080/echo"});
if(ws.send("hello"))
{
    if(auto reply = ws.recv())
    {
        // …
    }
}
// or: ws.read_loop([](std::string_view msg) { … });
ws.close();
```

## MCP over SSE

`http::mcp::server` serves the legacy MCP HTTP+SSE transport (compatible with
`mcp.server.sse.SseServerTransport`) on an `http::server`:

1. `GET /sse` → `event: endpoint` with `/messages/?session_id=<uuid4 hex>`
2. `POST` JSON-RPC to that URI → `202 Accepted`
3. Replies as SSE `event: message`

It runs `initialize` / `ping` / `tools/list` / `tools/call` and applies Host/Origin
allowlists (localhost defaults; disable with `dns_rebinding_protection(false)`).
Apps supply the tool callbacks; `attach` registers the routes. Keep the `mcp` object
alive for the lifetime of `listen`. Low-level `sse_transport` remains available for
custom session loops.

```cpp
import net;
import std;

try
{
    auto http = http::server{};
    auto mcp = http::mcp::server{"/messages/"};
    mcp.info({.name = "demo", .version = "1.0.0"})
        .list_tools([] {
            return std::vector<http::mcp::tool_spec>{
                {.name = "health", .description = "liveness"},
            };
        })
        .call_tool([](std::string_view, std::string_view) {
            return http::mcp::tool_result{.text = "ok"};
        })
        .attach(http, "/sse");
    http.listen("127.0.0.1", "8080");
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}
```

JSON-RPC dispatch uses a small in-module scanner, not an external JSON library. Keys
are matched only as top-level object members (depth-aware), so nested fields such as
`params.arguments.name` cannot shadow `params.name` when clients serialize maps
alphabetically. This keeps net4cpp network-only (plus `tester`); apps like YarDB
already depend on `xson` and can parse richer payloads in tool callbacks. Background:
[`docs/sse-mcp-implementation-plan.md`](docs/sse-mcp-implementation-plan.md).

## Syslog stream

### Basic usage

```cpp
import net;
import std;

using namespace net;
using namespace std::string_literals;

slog.log_level(syslog::severity::info);
slog.facility(syslog::facility::local0);
slog.app_name("example");

auto clothes = "shirts"s; auto spouse = "wife"; auto wrong = false;

slog << debug   << "... " << 3 << ' ' << 2 << ' ' << 1 << " Liftoff" << flush;

slog << info    << "The papers want to know whose " << clothes << " you wear..." << flush;

slog << notice  << "Tell my " << spouse << " I love her very much!" << flush;

slog << warning << "Ground Control to Major Tom Your circuit's dead, there's something " << wrong << '?' << flush;

slog << error   << "Planet Earth is blue and there's nothing I can do." << flush;
```

### Fluent configuration

The structured log stream supports method chaining for convenient configuration:

```cpp
slog.format(net::log_format::jsonl)
    .app_name("my-service")
    .log_level(syslog::severity::info)
    .sd_id("app")
    .redirect("logs/app.log");
```

All configuration methods return `structured_log_stream&` to enable fluent chaining.

### Structured fields

```cpp
slog << info << std::pair{"user_id", 42} << std::pair{"ip", "192.168.1.1"} << "User logged in" << flush;
```

### Source location

Automatically capture file, line, and function information:

```cpp
slog << info << std::source_location::current() << "Operation completed" << flush;

// Or with structured fields
slog << error
     << std::source_location::current()
     << std::pair{"duration_ms", 127}
     << "Request failed"
     << flush;
```

### Check the log level before logging

```cpp
if(slog.is_enabled(syslog::severity::debug))
{
    // Expensive debug computation
    auto details = compute_debug_info();
    slog << debug << details << flush;
}
```
