#pragma once

#include <string>
#include "net/endpointstream.hpp"

namespace net {

class receiver
{
public:

    receiver(const std::string& group, const std::string& service) :
    m_group{group},
    m_service{service}
    {};

    iendpointstream join();

    void leave();

    const auto& group() const
    {
        return m_group;
    }

    const auto& service() const
    {
        return m_service;
    }

private:

    std::string m_group;

    std::string m_service;
};

iendpointstream join(const std::string& group, const std::string& service, bool loop = true);

} // namespace net
