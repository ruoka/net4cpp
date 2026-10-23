// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import :endpointbuf;
import :endpointstream;
import :posix;
import :socket;
import tester;
import std;

using namespace net;

namespace {
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

// Local stream pair. socketpair is AF_UNIX on both Linux and macOS, so these
// tests do not depend on a network interface.
struct stream_pair
{
    socket local;
    socket peer;
};

inline std::optional<stream_pair> connected_stream_pair()
{
    int fds[2] = {-1, -1};
    if(posix::socketpair(posix::af_unix, posix::sock_stream, 0, fds) != 0)
        return std::nullopt;
    return stream_pair{socket{fds[0]}, socket{fds[1]}};
}
}

auto register_endpointbuf_tests()
{
    tester::bdd::scenario("Buffer size constants, [net]") = [] {
        tester::bdd::given("TCP and UDP buffer sizes") = [] {
            check_eq(tcp_buffer_size, 4096u);
            check_eq(udp_buffer_size, 65507u);
        };
    };

    tester::bdd::scenario("endpointbuf_base construction, [net]") = [] {
        tester::bdd::given("A socket") = [] {
            if(not network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            check_nothrow([&s] {
                endpointbuf_base buf{std::move(s)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("endpointbuf_base wait_for, [net]") = [] {
        tester::bdd::given("An endpointbuf_base with a new socket") = [] {
            if(not network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            endpointbuf_base buf{std::move(s)};
            
            tester::bdd::when("Calling wait_for with a short timeout") = [&buf] {
                using namespace std::chrono_literals;
                auto result = buf.wait_for(1ms);
                check_true(not result);
            };
        };
    };

    tester::bdd::scenario("endpointbuf template construction, [net]") = [] {
        tester::bdd::given("A socket") = [] {
            if(not network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            check_nothrow([&s] {
                endpointbuf<1024> buf{std::move(s)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("Type aliases, [net]") = [] {
        tester::bdd::given("TCP and UDP endpointbuf type aliases") = [] {
            if(not network_tests_enabled()) return;
            
            auto s1 = socket{posix::af_inet, posix::sock_stream};
            auto s2 = socket{posix::af_inet, posix::sock_dgram};
            
            check_nothrow([&s1, &s2] {
                tcp_endpointbuf tcp_buf{std::move(s1)};
                udp_endpointbuf udp_buf{std::move(s2)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("endpointbuf buffer initialization, [net]") = [] {
        tester::bdd::given("An endpointbuf with a socket") = [] {
            if(not network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            endpointbuf<512> buf{std::move(s)};
            
            tester::bdd::when("Checking buffer state") = [] {
                // After construction, the buffer should be ready for use
                // We can't directly access protected members, but we can verify
                // the object was constructed successfully
                check_true(true);
            };
        };
    };

    tester::bdd::scenario("endpointbuf construction with different sizes, [net]") = [] {
        tester::bdd::given("Sockets for different buffer sizes") = [] {
            if(not network_tests_enabled()) return;
            
            tester::bdd::when("Creating endpointbuf with different buffer sizes") = [] {
                check_nothrow([] {
                    auto s1 = socket{posix::af_inet, posix::sock_stream};
                    auto s2 = socket{posix::af_inet, posix::sock_stream};
                    auto s3 = socket{posix::af_inet, posix::sock_stream};
                    endpointbuf<256> buf1{std::move(s1)};
                    endpointbuf<512> buf2{std::move(s2)};
                    endpointbuf<1024> buf3{std::move(s3)};
                    check_true(true);
                });
            };
        };
    };

    tester::bdd::scenario("endpointbuf read_timeout stops an idle recv, [net]") = [] {
        tester::bdd::given("A connected stream pair and a 200ms read timeout") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            tcp_endpointbuf buf{std::move(ends->local)};
            buf.read_timeout(std::chrono::milliseconds{200});
            std::istream in{&buf};

            tester::bdd::when("No bytes arrive") = [&] {
                using namespace std::chrono_literals;
                const auto start = std::chrono::steady_clock::now();
                const auto ch = in.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;

                tester::bdd::then("The read ends with the timeout flag set") = [&] {
                    check_eq(ch, std::char_traits<char>::eof());
                    check_true(buf.read_timed_out());
                    check_true(elapsed >= 150ms);
                    check_true(elapsed < 2s);
                };
            };
        };
    };

    tester::bdd::scenario("endpointbuf read_timeout does not fire when the peer sends, [net]") = [] {
        tester::bdd::given("A connected stream pair") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            const char payload[] = {'h', 'i'};
            const auto sent = posix::send(ends->peer, payload, sizeof payload, posix::msg_nosignal);
            check_eq(sent, static_cast<decltype(sent)>(sizeof payload));

            tcp_endpointbuf buf{std::move(ends->local)};
            buf.read_timeout(std::chrono::milliseconds{1000});
            std::istream in{&buf};

            tester::bdd::when("Both bytes are already buffered by the peer") = [&] {
                const auto first = in.get();
                const auto second = in.get();

                tester::bdd::then("The bytes are returned and the timeout flag stays clear") = [&] {
                    check_eq(first, std::char_traits<char>::to_int_type('h'));
                    check_eq(second, std::char_traits<char>::to_int_type('i'));
                    check_true(not buf.read_timed_out());
                };
            };
        };
    };

    tester::bdd::scenario("endpointbuf default read waits for a late byte, [net]") = [] {
        tester::bdd::given("A connected stream pair with no read timeout") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            tcp_endpointbuf buf{std::move(ends->local)};
            check_eq(buf.read_timeout().count(), 0);
            std::istream in{&buf};

            tester::bdd::when("The peer sends one byte after a short delay") = [&] {
                using namespace std::chrono_literals;
                std::thread sender{[&peer = ends->peer] {
                    std::this_thread::sleep_for(150ms);
                    const char byte = 'Z';
                    posix::send(peer, &byte, 1, posix::msg_nosignal);
                }};

                const auto start = std::chrono::steady_clock::now();
                const auto ch = in.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;
                sender.join();

                tester::bdd::then("The read blocks until the byte arrives") = [&] {
                    check_eq(ch, std::char_traits<char>::to_int_type('Z'));
                    check_true(not buf.read_timed_out());
                    check_true(elapsed >= 100ms);
                    check_true(elapsed < 2s);
                };
            };
        };
    };

    tester::bdd::scenario("endpointbuf read_timeout applies to a large read after the first bytes, [net]") = [] {
        tester::bdd::given("One buffered byte and a request larger than the streambuf") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            const char byte = 'Q';
            const auto sent = posix::send(ends->peer, &byte, 1, posix::msg_nosignal);
            check_eq(sent, static_cast<decltype(sent)>(1));

            tcp_endpointbuf buf{std::move(ends->local)};
            buf.read_timeout(std::chrono::milliseconds{200});
            std::istream in{&buf};
            std::array<char, 8192> dest{};

            tester::bdd::when("The read asks for more than the streambuf") = [&] {
                using namespace std::chrono_literals;
                const auto start = std::chrono::steady_clock::now();
                in.read(dest.data(), static_cast<std::streamsize>(dest.size()));
                const auto elapsed = std::chrono::steady_clock::now() - start;

                tester::bdd::then("The first byte is kept and the rest times out") = [&] {
                    check_eq(in.gcount(), std::streamsize{1});
                    check_eq(dest[0], 'Q');
                    check_true(buf.read_timed_out());
                    check_true(elapsed >= 150ms);
                    check_true(elapsed < 2s);
                };
            };
        };
    };

    tester::bdd::scenario("endpointbuf bytes already buffered are not subject to read_timeout, [net]") = [] {
        tester::bdd::given("Two bytes delivered in one recv") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            const char payload[] = {'a', 'b'};
            const auto sent = posix::send(ends->peer, payload, sizeof payload, posix::msg_nosignal);
            check_eq(sent, static_cast<decltype(sent)>(sizeof payload));

            tcp_endpointbuf buf{std::move(ends->local)};
            buf.read_timeout(std::chrono::milliseconds{500});
            std::istream in{&buf};
            check_eq(in.get(), std::char_traits<char>::to_int_type('a'));

            tester::bdd::when("The second byte is taken from the get area") = [&] {
                using namespace std::chrono_literals;
                const auto start = std::chrono::steady_clock::now();
                const auto ch = in.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;

                tester::bdd::then("It returns without waiting for the socket") = [&] {
                    check_eq(ch, std::char_traits<char>::to_int_type('b'));
                    check_true(not buf.read_timed_out());
                    check_true(elapsed < 100ms);
                };
            };
        };
    };

    tester::bdd::scenario("endpointbuf shutdown wakes a timed read, [net]") = [] {
        tester::bdd::given("A read blocked on a long timeout") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            tcp_endpointbuf buf{std::move(ends->local)};
            buf.read_timeout(std::chrono::milliseconds{2000});
            std::istream in{&buf};

            tester::bdd::when("shutdown is called while the read is waiting") = [&] {
                using namespace std::chrono_literals;
                std::thread stopper{[&buf] {
                    std::this_thread::sleep_for(50ms);
                    buf.shutdown();
                }};

                const auto start = std::chrono::steady_clock::now();
                const auto ch = in.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;
                stopper.join();

                tester::bdd::then("The read ends as a close, not as an idle timeout") = [&] {
                    check_eq(ch, std::char_traits<char>::eof());
                    check_true(not buf.read_timed_out());
                    check_true(elapsed < 1s);
                };
            };
        };
    };

    tester::bdd::scenario("endpointstream forwards read_timeout to the buffer, [net]") = [] {
        tester::bdd::given("An endpointstream over a connected pair") = [] {
            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;

            auto stream = endpointstream{new tcp_endpointbuf{std::move(ends->local)}};
            stream.read_timeout(std::chrono::milliseconds{200});

            tester::bdd::when("No bytes arrive") = [&] {
                using namespace std::chrono_literals;
                const auto start = std::chrono::steady_clock::now();
                const auto ch = stream.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;

                tester::bdd::then("The stream reports the buffer timeout") = [&] {
                    check_eq(stream.read_timeout().count(), 200);
                    check_eq(ch, std::char_traits<char>::eof());
                    check_true(stream.read_timed_out());
                    check_true(elapsed >= 150ms);
                    check_true(elapsed < 2s);
                };
            };
        };
    };

    // select() could not wait on descriptors >= FD_SETSIZE (1024): FD_SET wrote
    // past the fd_set on Linux and was a silent timeout on macOS. Containers
    // commonly allow far more open files, so such descriptors are reachable.
    tester::bdd::scenario("Reads with a timeout work on a descriptor above 1024, [net]") = [] {
        tester::bdd::given("A connected pair whose local end is descriptor >= 1100") = [] {
            constexpr auto high_fd = 1100;
            auto limit = posix::rlimit{};
            check_true(posix::getrlimit(posix::rlimit_nofile, &limit) == 0);
            const auto previous = limit;
            if(limit.rlim_cur <= high_fd)
            {
                limit.rlim_cur = std::min<decltype(limit.rlim_max)>(limit.rlim_max, 2048);
                if(limit.rlim_cur <= high_fd or posix::setrlimit(posix::rlimit_nofile, &limit) != 0)
                    return; // The OS will not hand out such a descriptor here.
            }

            auto ends = connected_stream_pair();
            check_true(ends.has_value());
            if(not ends)
                return;
            const auto fd = posix::fcntl(static_cast<int>(ends->local), posix::f_dupfd, high_fd);
            check_true(fd >= high_fd);
            if(fd < high_fd)
                return;
            ends->local.close();

            tcp_endpointbuf buf{socket{fd}};
            buf.read_timeout(std::chrono::milliseconds{200});
            std::istream in{&buf};

            tester::bdd::when("The peer sends a byte, then goes quiet") = [&] {
                using namespace std::chrono_literals;
                const char byte = 'x';
                posix::send(ends->peer, &byte, 1, posix::msg_nosignal);
                const auto first = in.get();

                const auto start = std::chrono::steady_clock::now();
                const auto second = in.get();
                const auto elapsed = std::chrono::steady_clock::now() - start;

                tester::bdd::then("The byte arrives and the quiet period times out normally") = [&] {
                    check_eq(first, std::char_traits<char>::to_int_type('x'));
                    check_eq(second, std::char_traits<char>::eof());
                    check_true(buf.read_timed_out());
                    check_true(elapsed >= 150ms);
                    check_true(elapsed < 2s);
                };
            };

            posix::setrlimit(posix::rlimit_nofile, &previous);
        };
    };

    return true;
}

const auto _ = register_endpointbuf_tests();

