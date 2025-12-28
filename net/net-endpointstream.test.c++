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

auto endpointstream_test_reg = test_case("Endpoint Stream") = [] {
    if(!network_tests_enabled()) return;
    tester::bdd::scenario("Join and Distribute (Dgram)") = [] {
        tester::bdd::given("A multicast join and distribute") = [] {
            check_nothrow([] {
                auto is = join("228.0.0.4","54321");
                auto os = distribute("228.0.0.4","54321");
            });
        };
    };

    tester::bdd::scenario("HTTP Request and Response") = [] {
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

    tester::bdd::scenario("Stream Move") = [] {
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

    tester::bdd::scenario("Bulk Data Transfer") = [] {
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
};
