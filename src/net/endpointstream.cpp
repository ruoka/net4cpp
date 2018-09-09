#include "net/endpointstream.hpp"
#include "net/endpointbuf.hpp"

namespace net {

    iendpointstream::iendpointstream(endpointbuf_base* sb) : std::istream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }

    iendpointstream::iendpointstream(iendpointstream&& s) : std::istream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }

    iendpointstream::~iendpointstream()
    {
        delete m_buf.get();
    }

    iendpointstream& iendpointstream::operator = (iendpointstream&& s)
    {
        swap(s);
        m_buf = std::move(s.m_buf);
        init(m_buf.get());
        return *this;
    }

    bool iendpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

    oendpointstream::oendpointstream(endpointbuf_base* sb) : std::ostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }

    oendpointstream::oendpointstream(oendpointstream&& s) : std::ostream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }

    oendpointstream::~oendpointstream()
    {
        delete m_buf.get();
    }

    oendpointstream& oendpointstream::operator = (oendpointstream&& s)
    {
        swap(s);
        m_buf = std::move(s.m_buf);
        init(m_buf.get());
        return *this;
    }

    bool oendpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

    endpointstream::endpointstream(endpointbuf_base* sb) : std::iostream{sb}, m_buf{sb}
    {
        init(m_buf.get());
    }

    endpointstream::endpointstream(endpointstream&& s) : std::iostream{std::move(s)}, m_buf{std::move(s.m_buf)}
    {
        init(m_buf.get());
    }

    endpointstream::~endpointstream()
    {
        delete m_buf.get();
    }

    endpointstream& endpointstream::operator = (endpointstream&& s)
    {
        swap(s);
        m_buf = std::move(s.m_buf);
        init(m_buf.get());
        return *this;
    }

    bool endpointstream::wait_for(const std::chrono::milliseconds& timeout) const
    {
        return m_buf->wait_for(timeout);
    }

} // namespace net
