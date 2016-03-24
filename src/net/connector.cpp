#include <system_error>
#include "net/connector.hpp"
#include "net/address_info.hpp"
#include "net/endpointbuf.hpp"

namespace net
{

connector::connector(const std::string& host, const std::string& service_or_port) :
m_host{host},
m_service_or_port{service_or_port},
m_timeout{default_connect_timeout}
{}

endpointstream connector::connect() const
{
    return net::connect(m_host, m_service_or_port, m_timeout);
}

endpointstream connect(const std::string& host, const std::string& service_or_port, const std::chrono::milliseconds& timeout)
{
    const net::address_info remote_address{host, service_or_port, SOCK_STREAM};
    for(const auto& address : remote_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        auto status = net::connect(s, address.ai_addr, address.ai_addrlen, timeout);
        //auto status = net::connect(s, address.ai_addr, address.ai_addrlen);
        if(status < 0)
            continue;

        return new endpointbuf<tcp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category()};
}

} // namespace net
