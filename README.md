# net4cpp
Network library based on **C++23 modules**.

## Requirements

- **Clang 21+** on Linux (`clang++-21`; CB and CI default to LLVM 21)
- **macOS**: locally built LLVM at `/usr/local/llvm` (see `deps/tester` README)
- Open in a dev container (`.devcontainer/`) for a reproducible Linux toolchain

This subproject is designed to work both:
- **Standalone** (as its own repo, with `deps/tester` available), and
- **As a dependency inside YarDB** (reusing YarDB’s `deps/tester`).

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

# Echo Server Example

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

# Http Request Example

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

# HTTP Escape Example

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

# WebSocket (v1 spike)

Upgrade is handled inside `http::server` (not as response middleware). Register a path
with `server.ws(...).ws(handler)`; a matching `GET` with `Upgrade: websocket` returns
`101` and runs a text-frame session (ping/pong/close + optional text replies).

Clients use `websocket::connect` for the same text session (masked outbound frames,
automatic ping/pong, `send` / `recv` / `read_loop` / `close`). No `wss://` — terminate
TLS in front of the server if needed.

v1 framing policy (fail closed with a close frame, no silent drops):

- Client-to-server frames must be masked (`1002` if not).
- Complete text frames only — `FIN=0`, `continuation`, and `binary` → `1003`.
- Text payloads must be valid UTF-8 (`1007`); no xson dependency.
- Control frames must be FIN and ≤ 125 bytes; RSV bits must be 0 (`1002`).
- Payloads larger than 1 MiB → `1009`.

```cpp
import net;
import std;

// Server
auto server = http::server{};
server.ws("/events").ws([](std::string_view msg) -> std::optional<std::string> {
    return std::string{msg}; // echo
});
server.listen("127.0.0.1", "8080");

// Client
auto ws = net::websocket::connect("127.0.0.1", "8080", "/events");
ws.send("hello");
if(auto reply = ws.recv())
{
    // …
}
// or: ws.read_loop([](std::string_view msg) { … });
ws.close();
```

# Syslog Stream Example

## Basic Usage

```cpp
import net;
import std;

using namespace net;
using namespace std::string_literals;

slog.level(syslog::severity::info);
slog.facility(syslog::facility::local0);
slog.appname("example");

auto clothes = "shirts"s; auto spouse = "wife"; auto wrong = false;

slog << debug   << "... " << 3 << ' ' << 2 << ' ' << 1 << " Liftoff" << flush;

slog << info    << "The papers want to know whose " << clothes << " you wear..." << flush;

slog << notice  << "Tell my " << spouse << " I love her very much!" << flush;

slog << warning << "Ground Control to Major Tom Your circuit's dead, there's something " << boolalpha << wrong << '?' << flush;

slog << error   << "Planet Earth is blue and there's nothing I can do." << flush;
```

## Fluent Configuration API

The structured log stream supports method chaining for convenient configuration:

```cpp
slog.format(net::log_format::jsonl)
    .app_name("my-service")
    .log_level(syslog::severity::info)
    .sd_id("app")
    .redirect("logs/app.log");
```

All configuration methods return `structured_log_stream&` to enable fluent chaining.

## Structured Logging with Fields

```cpp
// Using pair syntax
slog << info << std::pair{"user_id", 42} << std::pair{"ip", "192.168.1.1"} << "User logged in" << flush;

// Using field() convenience method
slog << info << slog.field("user_id", 42) << slog.field("ip", "192.168.1.1") << "User logged in" << flush;
```

## Source Location Support (C++20)

Automatically capture file, line, and function information:

```cpp
slog << info << std::source_location::current() << "Operation completed" << flush;

// Or with structured fields
slog << error 
     << std::source_location::current()
     << slog.field("duration_ms", 127)
     << "Request failed"
     << flush;
```

## Check Log Level Before Logging

```cpp
if (slog.is_enabled(syslog::severity::debug)) {
    // Expensive debug computation
    auto details = compute_debug_info();
    slog << debug << details << flush;
}
```
