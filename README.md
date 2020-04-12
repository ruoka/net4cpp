# net4cpp
Network library based on C++20 standard

# Echo Server Example

```cpp
try
{
    auto ator = acceptor{"::1", "2112"}; // IPv6 localhost
    auto s = ator.accept();

    while(s)
    {
        auto echo = ""s;
        getline(s, echo);
        s << echo << endl;
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
try
{
    auto s = connect("http://www.google.com");

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

```cpp
#include "net/syslogstream.hpp"

[...]

slog.level(syslog::severity::info);
slog.facility(syslog::facility::local0);
slog.tag("example");

auto clothes = "shirts"s; auto spouse = "wife"; auto wrong = false;

slog << debug   << "... " << 3 << ' ' << 2 << ' ' << 1 << " Liftoff" << flush;

slog << info    << "The papers want to know whose " << clothes << " you wear..." << flush;

slog << notice  << "Tell my " << spouse << " I love her very much!" << flush;

slog << warning << "Ground Control to Major Tom Your circuit's dead, there's something " << boolalpha << wrong << '?' << flush;

slog << error   << "Planet Earth is blue and there's nothing I can do." << flush;
```
