#pragma once

#include <string>
#include <system_error>
#include "net/network.hpp"

namespace net {

class addrinfo_iterator
{
public:

    addrinfo_iterator(net::addrinfo* a) : m_current{a}
    {}

    addrinfo& operator * ()
    {
        return *m_current;
    }

    addrinfo_iterator& operator ++ ()
    {
        m_current = m_current->ai_next;
        return *this;
    }

    bool operator != (const addrinfo_iterator& i)
    {
        return m_current != i.m_current;
    }

private:
    net::addrinfo* m_current;
};

class address_info
{
public:

    address_info(const std::string& node, const std::string& service_or_port, int socktype, int flags = AI_ALL, int family = AF_UNSPEC) : m_addrinfo{nullptr}
    {
        const auto hints = net::addrinfo{flags,family,socktype,0,0,nullptr,nullptr,nullptr};
        const auto error = ::getaddrinfo(node.c_str(), service_or_port.c_str(), &hints, &m_addrinfo);
        if(error) throw std::system_error{error, std::system_category(), "address resolution failed"};
    }

    address_info(address_info&& ai) : m_addrinfo{ai.m_addrinfo}
    {
        ai.m_addrinfo = nullptr;
    }

    ~address_info()
    {
        if(m_addrinfo) ::freeaddrinfo(m_addrinfo);;
    }

    operator bool() const
    {
        return m_addrinfo;
    }

    const addrinfo* operator -> () const
    {
        return m_addrinfo;
    }

    friend addrinfo_iterator begin(const address_info& a)
    {
        return a.m_addrinfo;
    }

    friend addrinfo_iterator end(const address_info& a)
    {
        return nullptr;
    }

private:
    address_info(address_info&) = delete;
    address_info& operator = (const address_info&) = delete;
    address_info& operator = (const address_info&&) = delete;
    addrinfo* m_addrinfo;
};

} // namespace net
