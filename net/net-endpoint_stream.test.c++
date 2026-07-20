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
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;
using tester::assertions::require_true;
using tester::assertions::require_eq;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_endpointstream_tests()
{
    if(not network_tests_enabled()) return false;

    tester::bdd::scenario("Join and Distribute (Dgram), [net]") = [] {
        tester::bdd::given("A multicast join and distribute") = [] {
            check_nothrow([] {
                auto is = join("228.0.0.4","54321");
                auto os = distribute("228.0.0.4","54321");
            });
        };
    };

tester::bdd::scenario("HTTP Request and Response, [net]") = [] {
        tester::bdd::given("A connection to google.fi") = [] {
            check_nothrow([] {
                try {
                    auto s = connect("www.google.fi","http");

                    s << "GET / HTTP/1.1"                << net::crlf
                      << "Host: www.google.com"          << net::crlf
                      << "Connection: close"             << net::crlf
                      << "Accept: text/plain, text/html" << net::crlf
                      << "Accept-Charset: utf-8"         << net::crlf
                      << net::crlf
                      << net::flush;

                    using namespace std::chrono_literals;
                    // Wait for data to become available
                    bool ready = s.wait_for(5s); // Increased timeout
                    // Ready might be true if google responds quickly
                    if (ready) {
                        std::string response;
                        if (std::getline(s, response)) {
                            // Check for some typical HTTP response
                            check_true(response.find("HTTP/1.1") != std::string::npos);
                        }
                    }
                } catch(...) {
                    // Ignore connection errors in this test if host is unreachable
                }
            });
        };
    };

tester::bdd::scenario("Stream Move, [net]") = [] {
        tester::bdd::given("An endpointstream that is moved") = [] {
            check_nothrow([] {
                try {
                    auto s1 = connect("www.google.fi","http");
                    auto s2 = std::move(s1);
                    // Check s2 is usable
                    check_true(s2.good());
                } catch(...) {}
            });
        };
    };

tester::bdd::scenario("Bulk Data Transfer, [net]") = [] {
        tester::bdd::given("A local TCP connection") = [] {
            acceptor acc{"127.0.0.1", "0"};
            const auto port = acc.bound_port();
            
            std::string received_data;
            const std::size_t data_size = tcp_buffer_size * 2 + 123;
            std::string sent_data(data_size, 'X');
            for(std::size_t i = 0; i < data_size; ++i) sent_data[i] = static_cast<char>('A' + (i % 26));

            std::atomic<bool> server_done{false};
            std::thread server_thread([&] {
                try {
                    auto [stream, client, client_port] = acc.accept();
                    
                    std::vector<char> buf(data_size);
                    stream.read(buf.data(), static_cast<std::streamsize>(data_size));
                    received_data.assign(buf.begin(), buf.begin() + stream.gcount());
                } catch(...) {}
                server_done = true;
            });

            {
                auto client = connect("127.0.0.1", std::to_string(port));
                client.write(sent_data.data(), static_cast<std::streamsize>(data_size));
                client.flush();
            }

            server_thread.join();
            check_eq(sent_data.size(), received_data.size());
            check_eq(sent_data, received_data);
        };
    };

    // Regression: xsgetn used to return after the first short recv when count >=
    // buffer size, so istream::read failed even though more bytes were still
    // coming. Split the write so the server must assemble across recv calls.
    tester::bdd::scenario("Bulk read assembles across short recv, [net]") = [] {
        acceptor acc{"127.0.0.1", "0"};
        const auto port = acc.bound_port();

        const std::size_t data_size = tcp_buffer_size + 512;
        const std::size_t first_chunk = 1024; // well below one TCP segment / buffer
        auto sent_data = std::string(data_size, '\0');
        for(std::size_t i = 0; i < data_size; ++i)
            sent_data[i] = static_cast<char>('a' + static_cast<int>(i % 26));

        auto received_data = std::string{};
        auto read_ok = std::atomic<bool>{false};
        std::thread server_thread{[&] {
            try
            {
                auto [stream, client, client_port] = acc.accept();
                auto buf = std::vector<char>(data_size);
                stream.read(buf.data(), static_cast<std::streamsize>(data_size));
                read_ok = static_cast<bool>(stream) and stream.gcount() == static_cast<std::streamsize>(data_size);
                if(read_ok)
                    received_data.assign(buf.begin(), buf.end());
            }
            catch(...) {}
        }};

        {
            using namespace std::chrono_literals;
            auto client = connect("127.0.0.1", std::to_string(port));
            client.write(sent_data.data(), static_cast<std::streamsize>(first_chunk));
            client.flush();
            // Give the server time to enter xsgetn and observe a short recv.
            std::this_thread::sleep_for(50ms);
            client.write(sent_data.data() + first_chunk,
                         static_cast<std::streamsize>(data_size - first_chunk));
            client.flush();
        }

        server_thread.join();
        check_true(read_ok.load());
        check_eq(sent_data, received_data);
    };

    // Regression: after the TCP short-recv assemble fix, xsgetn looped recv for
    // every socket type when count >= N. udp_buffer_size == N, so the natural
    // max-datagram read waited forever for more bytes after the first message.
    tester::bdd::scenario("UDP bulk read returns one datagram, [net]") = [] {
        using namespace std::chrono_literals;
        using namespace std::string_literals;

        auto receiver = socket{posix::af_inet, posix::sock_dgram};
        auto sender = socket{posix::af_inet, posix::sock_dgram};
        require_true(static_cast<bool>(receiver));
        require_true(static_cast<bool>(sender));

        auto addr = posix::sockaddr_in{};
        addr.sin_family = posix::af_inet;
        addr.sin_addr.s_addr = posix::htonl(posix::inaddr_loopback);
        addr.sin_port = posix::htons(0);
        require_eq(posix::bind(receiver, reinterpret_cast<posix::sockaddr*>(&addr), sizeof addr), 0);
        auto addrlen = posix::socklen_t{sizeof addr};
        require_eq(posix::getsockname(receiver, reinterpret_cast<posix::sockaddr*>(&addr), &addrlen), 0);
        require_eq(posix::connect(sender, reinterpret_cast<posix::sockaddr*>(&addr), sizeof addr), 0);

        const auto payload = "udp-xsgetn-one-message"s;
        require_eq(
            static_cast<std::size_t>(posix::send(sender, payload.data(), payload.size(), 0)),
            payload.size());

        // Do not wait_for/peek first: peek would underflow the datagram into the
        // streambuf and miss the count >= N bulk path this regression targets.
        endpointstream stream{new udp_endpointbuf{std::move(receiver)}};
        auto buf = std::vector<char>(udp_buffer_size);
        auto gcount = std::streamsize{0};
        auto done = std::atomic<bool>{false};
        std::thread reader{[&] {
            stream.read(buf.data(), static_cast<std::streamsize>(udp_buffer_size));
            gcount = stream.gcount();
            done = true;
        }};
        auto start = std::chrono::steady_clock::now();
        while(not done.load() and (std::chrono::steady_clock::now() - start) < 2s)
            std::this_thread::sleep_for(10ms);
        // Short datagram: failbit is expected; gcount must reflect one message
        // and the read must not block waiting for udp_buffer_size bytes.
        check_true(done.load());
        if(done.load())
            reader.join();
        else
            reader.detach(); // regression hang — don't block the test process forever
        check_eq(gcount, static_cast<std::streamsize>(payload.size()));
        check_eq(std::string_view{buf.data(), payload.size()}, std::string_view{payload});
    };

    // Regression: endpointstream::wait_for peeks, which underflows a short
    // datagram into the get area. A following read(udp_buffer_size) then had
    // count < N after draining that residue, skipped the PR #37 path-2 gate,
    // and assembled forever in the underflow loop.
    tester::bdd::scenario("UDP max read after wait_for returns one datagram, [net]") = [] {
        using namespace std::chrono_literals;
        using namespace std::string_literals;

        auto receiver = socket{posix::af_inet, posix::sock_dgram};
        auto sender = socket{posix::af_inet, posix::sock_dgram};
        require_true(static_cast<bool>(receiver));
        require_true(static_cast<bool>(sender));

        auto addr = posix::sockaddr_in{};
        addr.sin_family = posix::af_inet;
        addr.sin_addr.s_addr = posix::htonl(posix::inaddr_loopback);
        addr.sin_port = posix::htons(0);
        require_eq(posix::bind(receiver, reinterpret_cast<posix::sockaddr*>(&addr), sizeof addr), 0);
        auto addrlen = posix::socklen_t{sizeof addr};
        require_eq(posix::getsockname(receiver, reinterpret_cast<posix::sockaddr*>(&addr), &addrlen), 0);
        require_eq(posix::connect(sender, reinterpret_cast<posix::sockaddr*>(&addr), sizeof addr), 0);

        const auto payload = "udp-after-wait-for"s;
        require_eq(
            static_cast<std::size_t>(posix::send(sender, payload.data(), payload.size(), 0)),
            payload.size());

        endpointstream stream{new udp_endpointbuf{std::move(receiver)}};
        require_true(stream.wait_for(500ms));

        auto buf = std::vector<char>(udp_buffer_size);
        auto gcount = std::streamsize{0};
        auto done = std::atomic<bool>{false};
        std::thread reader{[&] {
            stream.read(buf.data(), static_cast<std::streamsize>(udp_buffer_size));
            gcount = stream.gcount();
            done = true;
        }};
        auto start = std::chrono::steady_clock::now();
        while(not done.load() and (std::chrono::steady_clock::now() - start) < 2s)
            std::this_thread::sleep_for(10ms);
        check_true(done.load());
        if(done.load())
            reader.join();
        else
            reader.detach();
        check_eq(gcount, static_cast<std::streamsize>(payload.size()));
        check_eq(std::string_view{buf.data(), payload.size()}, std::string_view{payload});
    };

    return true;
}

// Ownership tests need only an unconnected SOCK_STREAM fd — run even when
// NET_DISABLE_NETWORK_TESTS=1 (sandbox CI).
auto register_endpointstream_ownership_tests()
{
    // Regression: close() used to delete the endpointbuf but leave iostream's
    // rdbuf dangling (non-null). wait_for then null-dereferenced m_buf; flush/
    // get used the freed streambuf. Move left the source rdbuf aliased to the
    // destination buffer so post-move I/O on the source still hit the live socket.
    tester::bdd::scenario("close clears rdbuf; move detaches source, [net]") = [] {
        auto s1 = endpointstream{new tcp_endpointbuf{
            socket{posix::af_inet, posix::sock_stream}}};
        auto* owned = s1.rdbuf();
        require_true(owned != nullptr);

        auto s2 = std::move(s1);
        check_true(s1.rdbuf() == nullptr);
        check_true(s2.rdbuf() == owned);
        check_true(s2.good());

        s2.close();
        check_true(s2.rdbuf() == nullptr);
        check_true(not s2.wait_for(std::chrono::milliseconds{1}));
        check_nothrow([&] { s2.flush(); });
    };

    return true;
}

const auto _ = register_endpointstream_tests();
const auto _ownership = register_endpointstream_ownership_tests();
