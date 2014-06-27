#pragma once

#include <string>
#include <chrono>
#include <list>
#include <iosfwd>
#include "net/socket.hpp"

namespace net
{

const std::chrono::minutes default_accept_timeout{1};

class acceptor
{
public:

    acceptor(const std::string& host, const std::string& service);

    explicit acceptor(const std::string& service);

    std::streambuf* accept();

    std::streambuf* accept(std::string& peer, std::string& service_or_port);

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

    int wait();

    std::string m_host;

    std::string m_service;

	std::chrono::milliseconds m_timeout;

	std::list<socket> m_sockets;
};

} // namespace net
