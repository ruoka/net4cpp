#pragma once

#include <iostream>
#include <memory>

namespace net
{

class iendpointstream : public std::istream
{
public:
    iendpointstream(std::streambuf* sb) : std::istream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    iendpointstream(iendpointstream&& s) : std::istream{std::move(s)}, m_buf{s.m_buf.release()}
    {
        init(m_buf.get());
    }
private:
    iendpointstream(iendpointstream&) = delete;
    iendpointstream& operator = (const iendpointstream&) = delete;
    iendpointstream& operator = (iendpointstream&&) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

class oendpointstream : public std::ostream
{
public:
    oendpointstream(std::streambuf* sb) : std::ostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    oendpointstream(oendpointstream&& s) : std::ostream{std::move(s)}, m_buf{s.m_buf.release()}
    {
        init(m_buf.get());
    }
private:
    oendpointstream(oendpointstream&) = delete;
    oendpointstream& operator = (const oendpointstream&) = delete;
    oendpointstream& operator = (oendpointstream&&) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

class endpointstream : public std::iostream
{
public:
    endpointstream(std::streambuf* sb) : std::iostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    endpointstream(endpointstream&& s) : std::iostream{std::move(s)}, m_buf{s.m_buf.release()}
    {
        init(m_buf.get());
    }
private:
    endpointstream(endpointstream&) = delete;
    endpointstream& operator = (const endpointstream&) = delete;
    endpointstream& operator = (endpointstream&& s) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

} // namespace net

