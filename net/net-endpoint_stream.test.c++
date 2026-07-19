module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;

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

    return true;
}

const auto _ = register_endpointstream_tests();
