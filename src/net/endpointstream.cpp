#include "net/endpointstream.hpp"
#include "net/endpointbuf.hpp"

namespace net {

    iendpointstream::iendpointstream(endpointbuf_base* sb) : std::istream{sb}, m_buf{sb}
    {
        init(m_buf);
    }

    iendpointstream::iendpointstream(iendpointstream&& s) : std::istream{std::move(s)}, m_buf{s.m_buf}
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
        swap(s);
        return *this;
    }

    bool iendpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

    oendpointstream::oendpointstream(endpointbuf_base* sb) : std::ostream{sb}, m_buf{sb}
    {
        init(m_buf);
    }

    oendpointstream::oendpointstream(oendpointstream&& s) : std::ostream{std::move(s)}, m_buf{s.m_buf}
    {
        init(m_buf);
        s.m_buf = nullptr;
    }

    oendpointstream::~oendpointstream()
    {
        if(m_buf != nullptr)
            delete m_buf;
    }

    oendpointstream& oendpointstream::operator = (oendpointstream&& s)
    {
        swap(s);
        return *this;
    }

    bool oendpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

    endpointstream::endpointstream(endpointbuf_base* sb) : std::iostream{sb}, m_buf{sb}
    {
        init(m_buf);
    }

    endpointstream::endpointstream(endpointstream&& s) : std::iostream{std::move(s)}, m_buf{s.m_buf}
    {
        init(m_buf);
        s.m_buf = nullptr;
    }

    endpointstream::~endpointstream()
    {
        if(m_buf != nullptr)
            delete m_buf;
    }

    endpointstream& endpointstream::operator = (endpointstream&& s)
    {
        swap(s);
        return *this;
    }

    bool endpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

} // namespace net
