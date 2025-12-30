# net4cpp
Network library based on **C++23 modules**.

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
