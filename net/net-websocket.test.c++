module net;
import tester;
import std;

using namespace net;
using namespace net::websocket;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::require_true;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

inline frame make_masked_text(std::string_view text, std::uint32_t key = 0x01020304u)
{
    auto f = make_text_frame(text);
    f.masked = true;
    f.masking_key = key;
    return f;
}

inline frame make_masked_close(std::uint32_t key = 0x01020304u)
{
    auto f = make_close_frame();
    f.masked = true;
    f.masking_key = key;
    return f;
}

} // namespace

auto register_websocket_tests()
{
    tester::bdd::scenario("sec_websocket_accept matches RFC 6455 example, [net]") = [] {
        // RFC 6455 §1.3
        const auto accept = sec_websocket_accept("dGhlIHNhbXBsZSBub25jZQ=="sv);
        check_eq(accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="s);
    };

    tester::bdd::scenario("websocket frame round-trip with client mask, [net]") = [] {
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_masked_text("hello"sv)));

        auto iss = std::istringstream{oss.str()};
        auto decoded = frame{};
        require_true(read_frame(iss, decoded));
        check_eq(decoded.op, opcode::text);
        check_true(decoded.fin);
        check_eq(payload_as_string(decoded), "hello"s);
    };

    tester::bdd::scenario("websocket frame supports 16-bit extended length, [net]") = [] {
        const auto payload = std::string(200, 'x');
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_masked_text(payload)));

        auto iss = std::istringstream{oss.str()};
        auto decoded = frame{};
        require_true(read_frame(iss, decoded));
        check_eq(payload_as_string(decoded), payload);
    };

    tester::bdd::scenario("http server websocket echo upgrade, [net]") = [] {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->ws("/echo").ws([](std::string_view msg) -> std::optional<std::string> {
            return std::string{msg};
        });

        std::promise<void> bound;
        auto bound_future = bound.get_future();
        std::thread server_thread{[server, &bound] {
            try
            {
                server->listen("127.0.0.1"sv, "18090"sv, [&bound] { bound.set_value(); });
            }
            catch(...)
            {
                try { bound.set_value(); } catch(...) {}
            }
        }};

        using namespace std::chrono_literals;
        require_true(bound_future.wait_for(3s) == std::future_status::ready);
        std::this_thread::sleep_for(50ms);

        auto client = connect("127.0.0.1", "18090");
        client << "GET /echo HTTP/1.1" << net::crlf
               << "Host: 127.0.0.1:18090" << net::crlf
               << "Upgrade: websocket" << net::crlf
               << "Connection: Upgrade" << net::crlf
               << "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" << net::crlf
               << "Sec-WebSocket-Version: 13" << net::crlf
               << net::crlf << net::flush;

        auto version = ""s;
        auto status_code = ""s;
        auto reason = ""s;
        client >> version >> status_code;
        std::getline(client, reason);
        check_eq(status_code, "101"s);

        auto hs = http::headers{};
        client >> hs >> net::crlf;
        check_true(hs.contains("sec-websocket-accept"));
        check_eq(hs["sec-websocket-accept"], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="s);

        require_true(write_frame(client, make_masked_text("ping-me"sv)));
        auto reply = frame{};
        require_true(read_frame(client, reply));
        check_eq(reply.op, opcode::text);
        check_eq(payload_as_string(reply), "ping-me"s);

        require_true(write_frame(client, make_masked_close()));
        auto close_reply = frame{};
        require_true(read_frame(client, close_reply));
        check_eq(close_reply.op, opcode::close);

        server->stop();
        if(server_thread.joinable())
            server_thread.join();
    };

    return true;
}

const auto _ = register_websocket_tests();
