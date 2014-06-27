#include <cerrno>
#include "net/network.hpp"

namespace net {

// This is C style low-level function that implements connect that supports timeouts
int connect(int fd, const net::sockaddr* address, net::socklen_t address_len, const std::chrono::milliseconds& timeout)
{
    // Set the socket into non-blocking mode
    const auto saved_flags = net::fcntl(fd, F_GETFL, 0);
    if(saved_flags < 0)
        return -1;

    auto error = net::fcntl(fd, F_SETFL, saved_flags|O_NONBLOCK);
    if(error < 0)
        return -1;

    // This call to connect will now return immediately
    const auto connect_error = net::connect(fd, address, address_len);
    const auto connect_errno = errno;

    error = net::fcntl(fd, F_SETFL, saved_flags);
    if(error < 0)
        return -1;

    // Return if the connect was successful
    if(connect_error == 0)
        return 0;

    // Return unless the connect is still in progress
    if(connect_errno != EINPROGRESS)
        return -1;

    // Wait for the connect to complete or timeout using select
    net::fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    if(timeout.count())
    {
        const auto  s = timeout.count() / 1000;
        const auto us = (timeout.count() % 1000) * 1000;
        net::timeval tv{static_cast<std::time_t>(s), static_cast<int>(us)};
        error = net::select(FD_SETSIZE, &readfds, &writefds, nullptr, &tv);
        // Return 0 means that select/connect timeouted
        if(error == 0)
        {
            errno = ETIMEDOUT;
            return -1;
        }
    }
    else
        error = net::select(FD_SETSIZE, &readfds, &writefds, nullptr, nullptr);

    if(error < 0)
        return -1;

    // Get the return code from the connect
    int result;
    net::socklen_t result_len{sizeof result};
    error = net::getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if(error < 0)
        return -1;

    // Result 0 means that connect succeeded, otherwise result contains the errno
    if(result > 0)
    {
        errno = result;
        return -1;
    }

    // Return the connect was successful
    return 0;
}

} // namespace net
