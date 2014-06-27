#include "net/socket.hpp"
#include "net/network.hpp"

namespace net
{

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
    if(m_fd != -1) ::close(m_fd);
}

bool socket::wait_for(const std::chrono::milliseconds& timeout) const
{
    net::fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    const auto  s = timeout.count() / 1000;
    const auto us = (timeout.count() % 1000) * 1000;
    net::timeval tv{static_cast<std::time_t>(s), static_cast<int>(us)};

    const auto result = net::select(m_fd+1, &fds, nullptr, nullptr, &tv);
    return result > 0;
}

} // namespace net
