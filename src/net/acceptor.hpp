#pragma once

#include <string>
#include <chrono>
#include <list>
#include "net/endpointstream.hpp"
#include "net/socket.hpp"

namespace net
{

const std::chrono::minutes default_accept_timeout{1};

class acceptor
{
public:

    acceptor(const std::string& host, const std::string& service_or_port);

    explicit acceptor(const std::string& service);

    endpointstream accept();

    endpointstream accept(std::string& peer, std::string& port);

    const std::string& host() const
    {
        return m_host;
    }

    const std::string& service_or_port() const
    {
        return m_service_or_port;
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

    int wait();

    std::string m_host;

    std::string m_service_or_port;

    std::chrono::milliseconds m_timeout;

    std::list<socket> m_sockets;
};

} // namespace net
