module net;
import :posix;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::check_true;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_posix_tests()
{
    if(!network_tests_enabled()) return false;

    tester::bdd::scenario("connect_with_timeout success, [net]") = [] {
        struct State {
            int listen_fd = -1;
            posix::sockaddr_in addr{};
            ~State() { if (listen_fd >= 0) posix::close(listen_fd); }
        };
        auto state = std::make_shared<State>();

        tester::bdd::given("A listening server") = [state] {
            state->listen_fd = posix::socket(posix::af_inet, posix::sock_stream, 0);
            check_true(state->listen_fd >= 0);

            state->addr.sin_family = posix::af_inet;
            state->addr.sin_addr.s_addr = posix::htonl(posix::inaddr_loopback);
            state->addr.sin_port = 0; // Random port

            check_eq(posix::bind(state->listen_fd, (posix::sockaddr*)&state->addr, sizeof(state->addr)), 0);
            check_eq(posix::listen(state->listen_fd, 1), 0);

            posix::socklen_t len = sizeof(state->addr);
            check_eq(posix::getsockname(state->listen_fd, (posix::sockaddr*)&state->addr, &len), 0);

            tester::bdd::when("Connecting with timeout") = [state] {
                int client_fd = posix::socket(posix::af_inet, posix::sock_stream, 0);
                check_true(client_fd >= 0);

                auto status = posix::connect_with_timeout(client_fd, (posix::sockaddr*)&state->addr, sizeof(state->addr), std::chrono::milliseconds{1000});
                check_eq(status, 0);

                posix::close(client_fd);
            };
        };
    };

    tester::bdd::scenario("connect_with_timeout failure - connection refused, [net]") = [] {
        auto addr = std::make_shared<posix::sockaddr_in>();
        addr->sin_family = posix::af_inet;
        addr->sin_addr.s_addr = posix::htonl(posix::inaddr_loopback);
        addr->sin_port = posix::htons(1); 

        tester::bdd::when("Connecting to a non-listening port") = [addr] {
            int client_fd = posix::socket(posix::af_inet, posix::sock_stream, 0);
            check_true(client_fd >= 0);

            auto status = posix::connect_with_timeout(client_fd, (posix::sockaddr*)addr.get(), sizeof(*addr), std::chrono::milliseconds{500});
            check_eq(status, -1);
            check_true(posix::get_errno() == posix::econnrefused || posix::get_errno() == posix::etimedout);

            posix::close(client_fd);
        };
    };

    tester::bdd::scenario("connect_with_timeout failure - timeout, [net]") = [] {
        auto addr = std::make_shared<posix::sockaddr_in>();
        addr->sin_family = posix::af_inet;
        posix::inet_pton(posix::af_inet, "10.255.255.1", &addr->sin_addr); 
        addr->sin_port = posix::htons(80);

        tester::bdd::when("Connecting with a short timeout to non-routable IP") = [addr] {
            int client_fd = posix::socket(posix::af_inet, posix::sock_stream, 0);
            check_true(client_fd >= 0);

            auto start = std::chrono::steady_clock::now();
            auto status = posix::connect_with_timeout(client_fd, (posix::sockaddr*)addr.get(), sizeof(*addr), std::chrono::milliseconds{200});
            auto end = std::chrono::steady_clock::now();
            
            check_eq(status, -1);
            check_eq(posix::get_errno(), posix::etimedout);
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            check_true(duration >= std::chrono::milliseconds{200});

            posix::close(client_fd);
        };
    };

    return true;
}

const auto _ = register_posix_tests();
