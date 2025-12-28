module;
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

module net;
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

auto posix_test_reg = test_case("POSIX utilities") = [] {
    if(!network_tests_enabled()) return;

    tester::bdd::scenario("connect_with_timeout success") = [] {
        // Setup in scenario scope to keep it alive for nested blocks if needed,
        // but remember that nested blocks run AFTER this lambda returns.
        // So we use shared state.
        struct State {
            int listen_fd = -1;
            sockaddr_in addr{};
            ~State() { if (listen_fd >= 0) ::close(listen_fd); }
        };
        auto state = std::make_shared<State>();

        tester::bdd::given("A listening server") = [state] {
            state->listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            check_true(state->listen_fd >= 0);

            state->addr.sin_family = AF_INET;
            state->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            state->addr.sin_port = 0; // Random port

            check_eq(::bind(state->listen_fd, (sockaddr*)&state->addr, sizeof(state->addr)), 0);
            check_eq(::listen(state->listen_fd, 1), 0);

            socklen_t len = sizeof(state->addr);
            check_eq(::getsockname(state->listen_fd, (sockaddr*)&state->addr, &len), 0);
        };

        tester::bdd::when("Connecting with timeout") = [state] {
            int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            check_true(client_fd >= 0);

            auto status = posix::connect_with_timeout(client_fd, (sockaddr*)&state->addr, sizeof(state->addr), std::chrono::milliseconds{500});
            check_eq(status, 0);

            ::close(client_fd);
        };
    };

    tester::bdd::scenario("connect_with_timeout failure - connection refused") = [] {
        auto addr = std::make_shared<sockaddr_in>();
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr->sin_port = htons(1); 

        tester::bdd::when("Connecting to a non-listening port") = [addr] {
            int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            check_true(client_fd >= 0);

            auto status = posix::connect_with_timeout(client_fd, (sockaddr*)addr.get(), sizeof(*addr), std::chrono::milliseconds{500});
            check_eq(status, -1);
            check_true(errno == ECONNREFUSED || errno == ETIMEDOUT);

            ::close(client_fd);
        };
    };

    tester::bdd::scenario("connect_with_timeout failure - timeout") = [] {
        auto addr = std::make_shared<sockaddr_in>();
        addr->sin_family = AF_INET;
        inet_pton(AF_INET, "10.255.255.1", &addr->sin_addr); 
        addr->sin_port = htons(80);

        tester::bdd::when("Connecting with a short timeout to non-routable IP") = [addr] {
            int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            check_true(client_fd >= 0);

            auto start = std::chrono::steady_clock::now();
            auto status = posix::connect_with_timeout(client_fd, (sockaddr*)addr.get(), sizeof(*addr), std::chrono::milliseconds{200});
            auto end = std::chrono::steady_clock::now();
            
            check_eq(status, -1);
            check_eq(errno, ETIMEDOUT);
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            check_true(duration >= std::chrono::milliseconds{200});

            ::close(client_fd);
        };
    };
};
