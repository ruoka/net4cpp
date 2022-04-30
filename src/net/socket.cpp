#include "net/socket.hpp"
#include "net/network.hpp"

namespace net {

socket::socket(int domain, int type, int protocol) : m_fd{native_handle_npos}
{
    m_fd = ::socket(domain, type, protocol);
}

socket::socket(int fd) : m_fd{fd}
{}

socket::socket(socket&& s) : m_fd{native_handle_npos}
{
    m_fd = s.m_fd;
    s.m_fd = native_handle_npos;
}

socket::~socket()
{
    if(m_fd != native_handle_npos) net::close(m_fd);
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
    // checks ready for read
    const auto result = net::select(m_fd+1, &fds, nullptr, nullptr, &tv);
    return result > 0;
}

bool socket::wait() const
{
    auto fds = net::fd_set{};
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    // checks ready for send
    const auto result = net::select(m_fd+1, nullptr, &fds, nullptr, nullptr);
    return result > 0;
}

} // namespace net
