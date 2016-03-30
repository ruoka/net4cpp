#pragma once

#include <chrono>

namespace net {

class socket
{
public:

    socket(int domain, int type, int protocol = 0);

    socket(int fd);

    socket(socket&& s);

    ~socket();

    bool wait_for(const std::chrono::milliseconds& timeout) const;

    operator int() const
    {
        return m_fd;
    }

    operator bool() const
    {
        return m_fd > 0;
    }

private:
    socket(socket&) = delete;
    socket& operator = (const socket&) = delete;
    socket& operator = (const socket&&) = delete;
    int m_fd;
};

} // namespace net
