module net;
import :posix;
import tester;
import std;


using namespace net;

namespace {
using tester::assertions::check_true;
using tester::assertions::check_eq;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_socket_tests()
{
    if(!network_tests_enabled()) return false;

    tester::bdd::scenario("Basic construction, [net]") = [] {
        tester::bdd::given("An IPv4 TCP socket") = [] {
            const net::socket s{posix::af_inet, posix::sock_stream};
            check_true(!(!s));
            int fd = s;
            check_true(fd > 0);

            auto yes = 1;
            auto e = posix::setsockopt(s, posix::sol_socket, posix::so_reuseaddr, &yes, sizeof yes);
            check_eq(e, 0);
        };

        tester::bdd::given("A socket constructed from a file descriptor") = [] {
            const auto s = net::socket{2112};
            check_true(!(!s));
            int fd = s;
            check_eq(fd, 2112);

            auto yes = 1;
            auto e = posix::setsockopt(s, posix::sol_socket, posix::so_reuseaddr, &yes, sizeof yes);
            // This should fail because 2112 is likely not a valid socket FD in this context
            check_eq(e, -1);
        };

        tester::bdd::given("A socket being moved") = [] {
            auto s1 = net::socket{posix::af_inet, posix::sock_dgram};
            check_true(!(!s1));
            int fd1 = s1;
            check_true(fd1 > 0);

            const auto s2 = net::socket{std::move(s1)};
            check_true(!s1);
            check_true(!(!s2));
            int fd2 = s2;
            check_eq(fd2, fd1);

            int fd1_after = s1;
            check_eq(fd1_after, net::native_handle_npos);
        };
    };

    tester::bdd::scenario("Socket wait_for, [net]") = [] {
        tester::bdd::given("A new TCP socket") = [] {
            const auto s = net::socket{posix::af_inet, posix::sock_stream};
            using namespace std::chrono_literals;
            auto b = s.wait_for(1ms);
            check_true(!b);
        };
    };

    return true;
}

const auto _ = register_socket_tests();
