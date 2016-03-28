#include <sys/types.h>
#include <unistd.h>
#include "net/network.hpp"
#include "syslogstream.hpp"

namespace net::syslog
{
    int getpid()
    {
        return ::getpid();
    }

    std::string gethostname()
    {
        char buffer[NI_MAXHOST];
        ::gethostname(buffer,NI_MAXHOST);
        return {buffer};
    }

} // namespace net::global
