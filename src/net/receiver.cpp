#include <cassert>
#include "net/receiver.hpp"
#include "net/address_info.hpp"
#include "net/stream_buffer.hpp"

namespace net
{

std::streambuf* receiver::join()
{
    return net::join(m_group, m_service);
}

void receiver::leave()
{
    return net::leave(m_group, m_service);
}

std::streambuf* join(const std::string& group, const std::string& service, bool loop, unsigned ttl)
{
    const net::address_info distribute_address{group, "", SOCK_DGRAM};
    const net::address_info local_address{"", service, SOCK_DGRAM, AI_PASSIVE, distribute_address->ai_family};
    for(const auto& address : local_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        int reuse{1};
        auto status = net::setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof reuse);
        if(status < 0)
            continue;

        char loop_{loop ? '1' : '0'};
        status = net::setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop_, sizeof loop_);
        if(status < 0)
            continue;

        unsigned ttl_{ttl};
        status = net::setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl_, sizeof ttl_);
        if(status < 0)
            continue;

        status = net::bind(s, address.ai_addr, address.ai_addrlen);
        if(status < 0)
            continue;

        if(address.ai_family == AF_INET)
        {
            net::ip_mreq mreq;

            std::memcpy(&mreq.imr_multiaddr,
                &reinterpret_cast<const net::sockaddr_in*>(distribute_address->ai_addr)->sin_addr,
                sizeof mreq.imr_multiaddr);

            mreq.imr_interface.s_addr = htonl(INADDR_ANY);

            status = net::setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof mreq);
            if(status < 0)
                continue;
        }
        else if (address.ai_family == AF_INET6)
        {
            net::ipv6_mreq mreq;

            std::memcpy(&mreq.ipv6mr_multiaddr,
                &reinterpret_cast<const net::sockaddr_in6*>(distribute_address->ai_addr)->sin6_addr,
                sizeof mreq.ipv6mr_multiaddr);

            mreq.ipv6mr_interface = 0;

            status = net::setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof mreq);
            if(status < 0)
                continue;
        }
        else
        {
            assert(false);
        }

        return new stream_buffer<udp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category()};
}

void leave(const std::string& group, const std::string& service)
{
    assert(false); // FIXME
}

} // namespace net
