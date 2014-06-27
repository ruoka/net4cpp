#include <system_error>
#include "net/connector.hpp"
#include "net/address_info.hpp"
#include "net/stream_buffer.hpp"

namespace net
{

connector::connector(const std::string& host, const std::string& service) :
m_host{host},
m_service{service},
m_timeout{default_connect_timeout}
{}

std::streambuf* connector::connect() const
{
    return net::connect(m_host, m_service, m_timeout);
}

std::streambuf* connect(const std::string& host, const std::string& service, const std::chrono::milliseconds& timeout)
{
    const net::address_info remote_address{host, service, SOCK_STREAM};
    for(const auto& address : remote_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        auto status = net::connect(s, address.ai_addr, address.ai_addrlen, timeout);
        //auto status = net::connect(s, address.ai_addr, address.ai_addrlen);
        if(status < 0)
            continue;

        return new stream_buffer<tcp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category()};
}

} // namespace net
