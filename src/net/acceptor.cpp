#include <system_error>
#include "net/acceptor.hpp"
#include "net/address_info.hpp"
#include "net/endpointbuf.hpp"

namespace net
{

acceptor::acceptor(const std::string& host, const std::string& service) :
m_host{host},
m_service{service},
m_timeout{default_accept_timeout},
m_sockets{}
{
    const net::address_info local_address{m_host, m_service, SOCK_STREAM, AI_PASSIVE};
    for(const auto& address : local_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        int yes{1};
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
        throw std::system_error{errno, std::system_category(), "Acceptor could not be bound"};
}

acceptor::acceptor(const std::string& service) : acceptor("localhost", service) {}

std::streambuf* acceptor::accept()
{
    const auto fd = wait();

    net::socket s = net::accept(fd, nullptr, nullptr);
    if(!s)
        throw std::system_error{errno, std::system_category()};

    return new endpointbuf<tcp_buffer_size>{std::move(s)};
}

std::streambuf* acceptor::accept(std::string& peer, std::string& service_or_port)
{
    const auto fd = wait();

    net::sockaddr_storage sas;
    net::socklen_t saslen = sizeof sas;
    net::socket s = net::accept(fd, reinterpret_cast<net::sockaddr*>(&sas), &saslen);
    if(!s)
        throw std::system_error{errno, std::system_category()};

    char hbuf[NI_MAXHOST];
    char sbuf[NI_MAXSERV];
    const auto status = net::getnameinfo(reinterpret_cast<net::sockaddr*>(&sas), saslen, hbuf, sizeof hbuf, sbuf, sizeof sbuf, 0);
    if(status)
        throw std::system_error{errno, std::system_category()};

    peer.assign(hbuf);
    service_or_port.assign(sbuf);
    return new endpointbuf<tcp_buffer_size>{std::move(s)};
}

int acceptor::wait()
{
    net::fd_set fds;
    FD_ZERO(&fds);
    for(const auto& fd : m_sockets) FD_SET(fd,&fds);

    if(m_timeout.count())
    {
        const auto  s = m_timeout.count() / 1000;
        const auto us = (m_timeout.count() % 1000) * 1000;
        net::timeval tv{static_cast<std::time_t>(s), static_cast<int>(us)};
        const auto result = net::select(FD_SETSIZE, &fds, nullptr, nullptr, &tv);
        if(!result)
            throw std::system_error{errno, std::system_category(), "Acceptor timeouted"};
    }
    else
        net::select(FD_SETSIZE, &fds, nullptr, nullptr, nullptr);

    for(const auto& fd : m_sockets)
        if(FD_ISSET(fd,&fds)) return fd;

    throw std::system_error{errno, std::system_category(), "Acceptor failed"};
}

} // namespace net
