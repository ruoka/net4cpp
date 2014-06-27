#include <system_error>
#include "net/sender.hpp"
#include "net/address_info.hpp"
#include "net/stream_buffer.hpp"

namespace net
{

std::streambuf* sender::distribute()
{
    return net::distribute(m_group, m_service);
}

std::streambuf* distribute(const std::string& group, const std::string& service)
{
    const net::address_info distribute_address{group, service, SOCK_DGRAM};
    for(const auto& address : distribute_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        const auto error = net::connect(s, address.ai_addr, address.ai_addrlen);
        if(error)
            continue;

        return new stream_buffer<udp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category(), "Multicaster could not connect"};
}

} // namespace net
