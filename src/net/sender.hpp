#pragma once

#include <string>
#include "net/endpointstream.hpp"

namespace net {

class sender
{
public:

    sender(std::string_view group, std::string_view service_or_port) :
    m_group{group},
    m_service_or_port{service_or_port}
    {};

    oendpointstream distribute();

    const auto& group() const
    {
        return m_group;
    }

    const auto& service_or_port() const
    {
        return m_service_or_port;
    }

private:

    std::string m_group;

    std::string m_service_or_port;
};

oendpointstream distribute(std::string_view group, std::string_view service_or_port, unsigned ttl = 1);

} // namespace net
