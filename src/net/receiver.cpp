#include <cassert>
#include "net/receiver.hpp"
#include "net/address_info.hpp"
#include "net/endpointbuf.hpp"

namespace net
{

iendpointstream receiver::join()
{
    return net::join(m_group, m_service);
}

void receiver::leave()
{
    assert(false); // FIXME
}

iendpointstream join(const std::string& group, const std::string& service, bool loop)
{
    const net::address_info distribution_address{group, "", SOCK_DGRAM};
    const net::address_info local_address{"", service, SOCK_DGRAM, AI_PASSIVE, distribution_address->ai_family};
    for(const auto& address : local_address)
    {
        net::socket s{address.ai_family, address.ai_socktype, address.ai_protocol};
        if(!s)
            continue;

        auto reuse = 1;
        auto status = net::setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof reuse);
        if(status < 0)
            continue;

        auto looop = loop ? '1' : '0';
        status = net::setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &looop, sizeof looop);
        if(status < 0)
            continue;

        status = net::bind(s, address.ai_addr, address.ai_addrlen);
        if(status < 0)
            continue;

        if(address.ai_family == AF_INET)
        {
            net::ip_mreq mreq;

            std::memcpy(&mreq.imr_multiaddr,
                &reinterpret_cast<const net::sockaddr_in*>(distribution_address->ai_addr)->sin_addr,
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
                &reinterpret_cast<const net::sockaddr_in6*>(distribution_address->ai_addr)->sin6_addr,
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

        return new endpointbuf<udp_buffer_size>{std::move(s)};
    }

    throw std::system_error{errno, std::system_category()};
}

} // namespace net
