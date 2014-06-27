#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

namespace net
{

using ::getsockopt;

using ::setsockopt;

using ::bind;

using ::listen;

using ::select;

using ::accept;

using ::fcntl;

using ::connect;

using ::getnameinfo;

using ::read;

using ::write;

using timeval = ::timeval;

using fd_set = ::fd_set;

using addrinfo = ::addrinfo;

using sockaddr_storage = ::sockaddr_storage;

using sockaddr = ::sockaddr;

using sockaddr_in = ::sockaddr_in;

using sockaddr_in6 = ::sockaddr_in6;

using ip_mreq = ::ip_mreq;

using ipv6_mreq = ::ipv6_mreq;

using socklen_t = ::socklen_t;

// This is C style low-level function that implements connect that supports timeouts
int connect(int fd, const net::sockaddr* address, net::socklen_t address_len, const std::chrono::milliseconds& timeout);

} // namespace net
