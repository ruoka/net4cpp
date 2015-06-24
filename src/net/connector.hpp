#pragma once

#include <string>
#include <chrono>
#include "net/endpointstream.hpp"

namespace net
{

const std::chrono::seconds default_connect_timeout{3};

class connector
{
public:

    connector(const std::string& host, const std::string& service);

    endpointstream connect() const;

    const std::string& host() const
    {
        return m_host;
    }

    const std::string& service() const
    {
        return m_service;
    }

    const std::chrono::milliseconds& timeout() const
    {
        return m_timeout;
    }

    void timeout(const std::chrono::milliseconds& timeout)
    {
        m_timeout = timeout;
    }

private:

    std::string m_host;

    std::string m_service;

    std::chrono::milliseconds m_timeout;
};

endpointstream connect(const std::string& host,
                         const std::string& service,
                         const std::chrono::milliseconds& timeout = default_connect_timeout);

} // namespace net
