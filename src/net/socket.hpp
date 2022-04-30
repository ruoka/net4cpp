#pragma once

#include <chrono>

namespace net {

using native_handle_type = int;

constexpr native_handle_type native_handle_npos = -1;

class socket
{
public:

    socket(int domain, int type, int protocol = 0);

    socket(native_handle_type fd);

    socket(socket&& s);

    ~socket();

    // checks ready for read
    bool wait_for(const std::chrono::milliseconds& timeout) const;

    // checks ready for send
    bool wait() const;

    operator native_handle_type() const
    {
        return m_fd;
    }

    operator bool() const
    {
        return m_fd != native_handle_npos;
    }

private:

    socket(socket&) = delete;

    socket& operator = (const socket&) = delete;

    socket& operator = (socket&&) = delete;

    native_handle_type m_fd;
};

} // namespace net
