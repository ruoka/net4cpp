#pragma once

#include <string>
#include "net/endpointstream.hpp"

namespace net
{

class sender
{
public:

    sender(const std::string& group, const std::string& service) :
    m_group{group},
    m_service{service}
    {};

    oendpointstream distribute();

    const std::string& group() const
    {
        return m_group;
    }

    const std::string& service() const
    {
        return m_service;
    }

private:

    std::string m_group;

    std::string m_service;    
};

oendpointstream distribute(const std::string& group, const std::string& service, unsigned ttl = 1);

} // namespace net
