#pragma once

#include <istream>
#include <memory>

namespace net
{

class endpointstream : public std::iostream
{
public:
    endpointstream(std::streambuf* sb) : std::iostream{sb}, m_buf{sb}
    {}
private:
    std::unique_ptr<std::streambuf> m_buf;
};

} // namespace net

