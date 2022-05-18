#include <system_error>
#include "net/acceptor.hpp"
#include "net/address_info.hpp"
#include "net/endpointbuf.hpp"

namespace net
{

acceptor::acceptor(std::string_view host, std::string_view service_or_port) :
m_host{host},
m_service_or_port{service_or_port},
m_timeout{default_accept_timeout},
m_sockets{}
{
    const auto local_address = net::address_info{m_host, m_service_or_port, SOCK_STREAM, AI_PASSIVE};
    for(const auto& address : local_address)
    {
        auto s = net::socket{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        auto yes = 1;
        auto status = net::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
        if(status < 0)
            continue;

        status = net::bind(s, address.ai_addr, address.ai_addrlen);
        if(status < 0)
            continue;

        status = net::listen(s, 5);
        if(status < 0)
            continue;

        m_sockets.push_back(std::move(s));
    }

    if(m_sockets.empty())
        throw std::system_error{errno, std::system_category(), "socket could not be bound"};
}

acceptor::acceptor(std::string_view service) : acceptor("localhost", service) {}

acceptor::acceptor(net::uri uri) : acceptor(uri.host, uri.port == "" ? uri.scheme : uri.port) {}

std::tuple<acceptor::stream,acceptor::client,acceptor::port> acceptor::accept()
{
    const auto fd = wait();

    auto sas = net::sockaddr_storage{};
    auto saslen = net::socklen_t{sizeof sas};
    net::socket s = net::accept(fd, reinterpret_cast<net::sockaddr*>(&sas), &saslen);
    if(!s)
        throw std::system_error{errno, std::system_category(),"accept failed"};

    #ifdef NET_USE_SO_NOSIGPIPE
    auto yes = 1;
    const auto status1 = net::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof yes);
    if(status1)
        throw std::system_error{errno, std::system_category(),"setsockopt failed"};
    #endif

    char hbuf[NI_MAXHOST];
    char sbuf[NI_MAXSERV];
    const auto status2 = net::getnameinfo(reinterpret_cast<net::sockaddr*>(&sas), saslen, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST);
    if(status2)
        throw std::system_error{errno, std::system_category(),"getnameinfo failed"};

    return {new endpointbuf<tcp_buffer_size>{std::move(s)}, hbuf, sbuf};
}

int acceptor::wait()
{
    auto fds = net::fd_set{};
    FD_ZERO(&fds);
    for(int fd : m_sockets) FD_SET(fd,&fds);

    if(m_timeout.count())
    {
        net::timeval tv {
            static_cast<decltype(tv.tv_sec)>(m_timeout.count() / 1000),
            static_cast<decltype(tv.tv_usec)>(m_timeout.count() % 1000 * 1000)
        };
        const auto result = net::select(FD_SETSIZE, &fds, nullptr, nullptr, &tv);
        if(!result)
            throw std::system_error{errno, std::system_category(), "accept timeouted"};
    }
    else
        net::select(FD_SETSIZE, &fds, nullptr, nullptr, nullptr);

    for(int fd : m_sockets)
        if(FD_ISSET(fd,&fds)) return fd;

    throw std::system_error{errno, std::system_category(), "accept failed"};
}

} // namespace net
