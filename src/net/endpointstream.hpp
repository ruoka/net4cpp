#pragma once

#include <iostream>
#include <ostream>
#include "gsl/owner.hpp"

namespace net {

class endpointbuf_base;

class iendpointstream : public std::istream
{
public:

    iendpointstream(endpointbuf_base* sb);

    iendpointstream(iendpointstream&& s);

    ~iendpointstream();

    iendpointstream& operator = (iendpointstream&& s);

    bool wait_for(const std::chrono::milliseconds& timeout) const;

private:

    iendpointstream(const iendpointstream&) = delete;

    iendpointstream& operator = (const iendpointstream&) = delete;

    gsl::owner<endpointbuf_base*> m_buf;
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

    oendpointstream(endpointbuf_base* sb);

    oendpointstream(oendpointstream&& s);

    ~oendpointstream();

    oendpointstream& operator = (oendpointstream&& s);

    bool wait_for(const std::chrono::milliseconds& timeout) const;

private:

    oendpointstream(const oendpointstream&) = delete;

    oendpointstream& operator = (const oendpointstream&) = delete;

    gsl::owner<endpointbuf_base*> m_buf;
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

    endpointstream(endpointbuf_base* sb);

    endpointstream(endpointstream&& s);

    ~endpointstream();

    endpointstream& operator = (endpointstream&& s);

    bool wait_for(const std::chrono::milliseconds& timeout) const;

private:

    endpointstream(const endpointstream&) = delete;

    endpointstream& operator = (const endpointstream&) = delete;

    gsl::owner<endpointbuf_base*> m_buf;
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
