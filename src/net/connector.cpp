#include <system_error>
#include "net/address_info.hpp"
#include "net/endpointbuf.hpp"
#include "net/connector.hpp"

namespace net {

using namespace std::string_literals;

connector::connector(std::string_view host, std::string_view service_or_port) :
m_host{host},
m_service_or_port{service_or_port},
m_timeout{default_connect_timeout}
{}

connector::connector(const uri& url) :
m_host{url.host},
m_service_or_port{url.port == ""s ? url.scheme : url.port},
m_timeout{default_connect_timeout}
{}

endpointstream connector::connect() const
{
    return net::connect(m_host, m_service_or_port, m_timeout);
}

endpointstream connect(std::string_view host, std::string_view service_or_port, const std::chrono::milliseconds& timeout)
{
    auto remote_address = net::address_info{host, service_or_port, SOCK_STREAM};
    for(const auto& address : remote_address)
    {
        auto s = net::socket{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        #ifdef NET_USE_SO_NOSIGPIPE
        auto yes = 1;
        const auto status1 = net::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof yes);
        if(status1 < 0)
            continue;
        #endif

        const auto status2 = net::connect(s, address.ai_addr, address.ai_addrlen, timeout);
        //const auto status2 = net::connect(s, address.ai_addr, address.ai_addrlen);
        if(status2 < 0)
            continue;

        return new endpointbuf<tcp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category()};
}

endpointstream connect(const uri& url, const std::chrono::milliseconds& timeout)
{
    const auto host = std::string{url.host};
    const auto port = std::string{url.port == ""s ? url.scheme : url.port};
    return connect(host,port,timeout);
}

} // namespace net
