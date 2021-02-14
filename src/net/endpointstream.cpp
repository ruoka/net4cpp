#include <iostream> // FIXME
#include "net/endpointstream.hpp"
#include "net/endpointbuf.hpp"

namespace net {

    iendpointstream::iendpointstream(endpointbuf_base* buf) : std::istream{buf}, m_buf{buf}
    {
        init(m_buf);
    }

    iendpointstream::iendpointstream(iendpointstream&& s) : std::istream{s.m_buf}, m_buf{s.m_buf}
    {
        init(m_buf);
        s.m_buf = nullptr;
    }

    iendpointstream::~iendpointstream()
    {
        if(m_buf != nullptr)
            delete m_buf;
    }

    iendpointstream& iendpointstream::operator = (iendpointstream&& s)
    {
        m_buf = s.m_buf;
        s.m_buf = nullptr;
        init(m_buf);
        return *this;
    }

    bool iendpointstream::wait_for(const std::chrono::milliseconds& timeout)
    {
        return m_buf->wait_for(timeout) && peek() != traits_type::eof();
    }

    oendpointstream::oendpointstream(endpointbuf_base* sb) : std::ostream{sb}, m_buf{sb}
    {
        init(m_buf);
    }

    oendpointstream::oendpointstream(oendpointstream&& s) : std::ostream{s.m_buf}, m_buf{s.m_buf}
    {
        s.m_buf = nullptr;
        init(m_buf);
    }

    oendpointstream::~oendpointstream()
    {
        if(m_buf != nullptr)
            delete m_buf;
    }

    oendpointstream& oendpointstream::operator = (oendpointstream&& s)
    {
        m_buf = s.m_buf;
        s.m_buf = nullptr;
        init(m_buf);
        return *this;
    }

    endpointstream::endpointstream(endpointbuf_base* buf) : std::iostream{buf}, m_buf{buf}
    {
        init(m_buf);
    }

    endpointstream::endpointstream(endpointstream&& s) : std::iostream{s.m_buf}, m_buf{s.m_buf}
    {
        s.m_buf = nullptr;
        init(m_buf);
    }

    endpointstream::~endpointstream()
    {
        if(m_buf != nullptr)
            delete m_buf;
    }

    endpointstream& endpointstream::operator = (endpointstream&& s)
    {
        m_buf = s.m_buf;
        s.m_buf = nullptr;
        init(m_buf);
        return *this;
    }

    bool endpointstream::wait_for(const std::chrono::milliseconds& timeout)
    {
        return m_buf->wait_for(timeout) && peek() != traits_type::eof();
    }

} // namespace net
