#pragma once

#include <iostream>
#include <ostream>
#include <memory>

namespace net {

class iendpointstream : public std::istream
{
public:
    iendpointstream(std::streambuf* sb) : std::istream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    iendpointstream(iendpointstream&& s) : std::istream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }
    iendpointstream& operator = (iendpointstream&& s)
    {
        swap(s);
        m_buf.swap(s.m_buf);
        init(m_buf.get());
        return *this;
    }
private:
    iendpointstream(const iendpointstream&) = delete;
    iendpointstream& operator = (const iendpointstream&) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

inline void swap(iendpointstream& lhs, iendpointstream& rhs)
{
    auto tmp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(tmp);
}

class oendpointstream : public std::ostream
{
public:
    oendpointstream(std::streambuf* sb) : std::ostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    oendpointstream(oendpointstream&& s) : std::ostream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }
    oendpointstream& operator = (oendpointstream&& s)
    {
        swap(s);
        m_buf.swap(s.m_buf);
        init(m_buf.get());
        return *this;
    }
private:
    oendpointstream(const oendpointstream&) = delete;
    oendpointstream& operator = (const oendpointstream&) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

inline void swap(oendpointstream& lhs, oendpointstream& rhs)
{
    auto tmp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(tmp);
}

class endpointstream : public std::iostream
{
public:
    endpointstream(std::streambuf* sb) : std::iostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }
    endpointstream(endpointstream&& s) : std::iostream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }
    endpointstream& operator = (endpointstream&& s)
    {
        swap(s);
        m_buf.swap(s.m_buf);
        init(m_buf.get());
        return *this;
    }
private:
    endpointstream(const endpointstream&) = delete;
    endpointstream& operator = (const endpointstream&) = delete;
    std::unique_ptr<std::streambuf> m_buf;
};

inline void swap(endpointstream& lhs, endpointstream& rhs)
{
    auto tmp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(tmp);
}

inline std::ostream& sp(std::ostream& os)
{
    os.put(' ');
    return os;
}

inline std::ostream& crlf(std::ostream& os)
{
    os.put('\r').put('\n');
    return os;
}

inline std::ostream& newl(std::ostream& os)
{
    os.put('\n');
    return os;
}

using std::endl;

using std::flush;

} // namespace net
