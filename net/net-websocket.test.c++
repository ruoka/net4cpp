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

inline frame make_masked_ping(std::string_view payload, std::uint32_t key = 0x01020304u)
{
    auto f = frame{};
    f.op = opcode::ping;
    f.masked = true;
    f.masking_key = key;
    f.payload.resize(payload.size());
    if(not payload.empty())
        std::memcpy(f.payload.data(), payload.data(), payload.size());
    return f;
}

inline frame make_masked_pong(std::uint32_t key = 0x01020304u)
{
    auto f = frame{};
    f.op = opcode::pong;
    f.masked = true;
    f.masking_key = key;
    return f;
}

// In-memory duplex stream: reads consume a prepared input buffer, writes are
// captured separately so run_text_session responses can be inspected without a
// socket. Keeping the two directions apart avoids the reader looping over freshly
// written response bytes (which a single shared buffer would do).
class duplex_buf : public std::streambuf
{
public:

    explicit duplex_buf(std::string input) : m_in{std::move(input)}
    {
        auto* base = m_in.data();
        setg(base, base, base + m_in.size());
    }

    const std::string& output() const noexcept { return m_out; }

protected:

    int_type overflow(int_type ch) override
    {
        if(ch != traits_type::eof())
            m_out.push_back(static_cast<char>(ch));
        return ch;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        m_out.append(s, static_cast<std::size_t>(n));
        return n;
    }

private:

    std::string m_in;
    std::string m_out;
};

class duplex_stream : public std::iostream
{
public:

    explicit duplex_stream(std::string input) : std::iostream{nullptr}, m_buf{std::move(input)}
    {
        rdbuf(&m_buf);
    }

    const std::string& output() const noexcept { return m_buf.output(); }

private:

    duplex_buf m_buf;
};

// Encode a frame to bytes for use as duplex_stream input.
inline std::string frame_bytes(const frame& f)
{
    auto oss = std::ostringstream{};
    write_frame(oss, f);
    return oss.str();
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

    tester::bdd::scenario("websocket unmasked server frame round-trip, [net]") = [] {
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_text_frame("server-say"sv)));

        auto iss = std::istringstream{oss.str()};
        auto decoded = frame{};
        require_true(read_frame(iss, decoded));
        check_true(not decoded.masked);
        check_eq(decoded.op, opcode::text);
        check_eq(payload_as_string(decoded), "server-say"s);
    };

    tester::bdd::scenario("websocket frame supports 64-bit extended length, [net]") = [] {
        // > 0xFFFF forces the 8-byte length encoding, still under the 1 MiB cap.
        const auto payload = std::string(70000, 'y');
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_masked_text(payload)));

        auto iss = std::istringstream{oss.str()};
        auto decoded = frame{};
        require_true(read_frame(iss, decoded));
        check_eq(decoded.payload.size(), payload.size());
        check_eq(payload_as_string(decoded), payload);
    };

    tester::bdd::scenario("websocket read_frame rejects oversized payload, [net]") = [] {
        // Hand-craft a header that claims > 1 MiB via the 64-bit length field.
        auto header = std::string{};
        header.push_back(static_cast<char>(0x81)); // FIN + text
        header.push_back(static_cast<char>(0x7F)); // 127 => 64-bit length, unmasked
        const auto len = std::uint64_t{(1u << 20) + 1};
        for(int shift = 56; shift >= 0; shift -= 8)
            header.push_back(static_cast<char>((len >> shift) & 0xFFu));

        auto iss = std::istringstream{header};
        auto decoded = frame{};
        check_true(not read_frame(iss, decoded));
    };

    tester::bdd::scenario("websocket read_frame fails on truncated payload, [net]") = [] {
        // Announce 5 bytes but only provide 2.
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_masked_text("hello"sv)));
        auto truncated = oss.str().substr(0, oss.str().size() - 3);

        auto iss = std::istringstream{truncated};
        auto decoded = frame{};
        check_true(not read_frame(iss, decoded));
    };

    tester::bdd::scenario("run_text_session echoes text via handler, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_text("hello"sv))};
        run_text_session(stream, [](std::string_view msg) -> std::optional<std::string> {
            return std::string{msg} + "!";
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply));
        check_true(not reply.masked);
        check_eq(reply.op, opcode::text);
        check_eq(payload_as_string(reply), "hello!"s);
    };

    tester::bdd::scenario("run_text_session suppresses reply when handler returns nullopt, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_text("quiet"sv))};
        run_text_session(stream, [](std::string_view) -> std::optional<std::string> {
            return std::nullopt;
        });
        check_true(stream.output().empty());
    };

    tester::bdd::scenario("run_text_session answers ping with pong, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_ping("hb"sv))};
        run_text_session(stream, nullptr);

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply));
        check_eq(reply.op, opcode::pong);
        check_eq(payload_as_string(reply), "hb"s);
    };

    tester::bdd::scenario("run_text_session ignores pong frames, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_pong())};
        run_text_session(stream, nullptr);
        check_true(stream.output().empty());
    };

    tester::bdd::scenario("run_text_session replies to close and ends, [net]") = [] {
        // A close frame followed by a text frame: the session must stop after the
        // close and never echo the trailing text.
        auto input = frame_bytes(make_masked_close());
        input += frame_bytes(make_masked_text("after-close"sv));

        auto stream = duplex_stream{input};
        run_text_session(stream, [](std::string_view msg) -> std::optional<std::string> {
            return std::string{msg};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply));
        check_eq(reply.op, opcode::close);

        auto extra = frame{};
        check_true(not read_frame(out, extra)); // nothing echoed after close
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
