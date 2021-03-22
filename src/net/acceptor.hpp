#pragma once

#include <string>
#include <chrono>
#include <list>
#include "net/endpointstream.hpp"
#include "net/socket.hpp"
#include "net/uri.hpp"

namespace net {

const std::chrono::minutes default_accept_timeout{1};

class acceptor
{
public:

    acceptor(std::string_view host, std::string_view service_or_port);

    explicit acceptor(std::string_view service);

    acceptor(net::uri uri);

    endpointstream accept();

    endpointstream accept(std::string& peer, std::string& port);

    const auto& host() const
    {
        return m_host;
    }

    const auto& service_or_port() const
    {
        return m_service_or_port;
    }

    const auto& timeout() const
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
