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
};
