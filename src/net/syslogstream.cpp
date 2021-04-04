#include "net/network.hpp"
#include "net/sender.hpp"
#include "net/connector.hpp"
#include "net/syslogstream.hpp"

namespace net::syslog {

    int getpid()
    {
        return ::getpid();
    }

    std::string gethostname()
    {
        char buffer[NI_MAXHOST];
        ::gethostname(buffer,NI_MAXHOST);
        return buffer;
    }

} // namespace net::syslog

namespace {

    auto syslogstream_builder()
    {
        try
        {
            return net::distribute("","syslog");
        }
        catch(...) {}
        try
        {
            return net::distribute("","514");
        }
        catch(...) {}
        try
        {
            auto fallback = net::oendpointstream{nullptr};
            fallback.rdbuf(std::clog.rdbuf());
            return fallback;
        }
        catch(...) {}
        std::terminate();
    }

}

namespace net {

    syslogstream slog = syslogstream{syslogstream_builder()};

} // namespace net
