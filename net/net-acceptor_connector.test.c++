module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::failed;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_acceptor_connector_tests()
{
    if(!network_tests_enabled()) return false;
    std::array<int, 100> data;
    for (int i = 0; i < 100; ++i) data[i] = i + 1;

    using namespace std::chrono_literals;

    auto received = std::vector<int>{};
    received.reserve(data.size());
    auto received_mutex = std::mutex{};
    std::atomic<bool> accept_failed{false};
    std::atomic<bool> connect_failed{false};
    std::atomic<bool> connector_timed_out{false};

    // Avoid std::async/std::future here: clang 22 has been observed to crash in <future>
    // instantiation in some module builds. net provides its own socket timeouts to keep
    // these threads bounded.
    std::thread t_acceptor{
        [&]{
            try {
                auto ator = net::acceptor{"localhost", "54321"};
                ator.timeout(2s);
                auto [os, host, port] = ator.accept();
                for(const auto v : data)
                    os << v << std::endl;
            } catch (...) {
                accept_failed = true;
            }
        }};

    std::thread t_connector{
        [&]{
            try {
                auto ctor = net::connector{"localhost", "54321"};
                ctor.timeout(2s);
                auto is = ctor.connect();
                for([[maybe_unused]] const auto expected : data)
                {
                    auto ii = 0;
                    if(is.wait_for(2s)) {
                        is >> ii;
                        auto lock = std::lock_guard<std::mutex>{received_mutex};
                        received.push_back(ii);
                    } else {
                        connector_timed_out = true;
                        break;
                    }
                }
            } catch (...) {
                connect_failed = true;
            }
        }};

    t_acceptor.join();
    t_connector.join();

    if(accept_failed) failed("Acceptor thread failed");
    if(connect_failed) failed("Connector thread failed");
    if(connector_timed_out) failed("Timeout waiting for data in connector");

    // Validate whatever we received, on the main thread.
    for(std::size_t idx = 0; idx < received.size(); ++idx)
        check_eq(data[idx], received[idx]);
    if(received.size() != data.size())
        failed("Connector did not receive all expected values");

    return true;
}

const auto _ = register_acceptor_connector_tests();
