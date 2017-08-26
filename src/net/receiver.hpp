#pragma once

#include <string>
#include "net/endpointstream.hpp"

namespace net {

class receiver
{
public:

    receiver(std::string_view group, std::string_view service) :
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

iendpointstream join(std::string_view group, std::string_view service, bool loop = true);

} // namespace net
