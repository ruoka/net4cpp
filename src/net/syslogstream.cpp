#include "net/network.hpp"
#include "net/sender.hpp"
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

} // namespace net::global

namespace net {

    syslogstream slog = syslogstream{distribute("","syslog")};

} // namespace net
