#include "net/socket.hpp"
#include "net/network.hpp"

namespace net {

socket::socket(int domain, int type, int protocol) : m_fd{-1}
{
    m_fd = ::socket(domain, type, protocol);
}

socket::socket(int fd) : m_fd{fd}
{}

socket::socket(socket&& s)
{
    m_fd = s.m_fd;
    s.m_fd = -1;
}

socket::~socket()
{
    if(m_fd != -1) net::close(m_fd);
}

bool socket::wait_for(const std::chrono::milliseconds& timeout) const
{
    auto fds = net::fd_set{};
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    net::timeval tv{
        static_cast<decltype(tv.tv_sec)>(timeout.count() / 1000),
        static_cast<decltype(tv.tv_usec)>(timeout.count() % 1000 * 1000)
    };
    const auto result = net::select(m_fd+1, &fds, nullptr, nullptr, &tv);
    return result > 0;
}

} // namespace net
