#pragma once

#include <string>
#include <iosfwd>

namespace net
{

class sender
{
public:

    sender(const std::string& group, const std::string& service) :
    m_group{group},
    m_service{service}
    {};

    std::streambuf* distribute();

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

std::streambuf* distribute(const std::string& group, const std::string& service);

} // namespace net
