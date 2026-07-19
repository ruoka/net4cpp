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
        require_true(read_frame(iss, decoded) == frame_read::ok);
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
        require_true(read_frame(iss, decoded) == frame_read::ok);
        check_eq(payload_as_string(decoded), payload);
    };

    tester::bdd::scenario("websocket unmasked server frame round-trip, [net]") = [] {
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_text_frame("server-say"sv)));

        auto iss = std::istringstream{oss.str()};
        auto decoded = frame{};
        require_true(read_frame(iss, decoded) == frame_read::ok);
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
        require_true(read_frame(iss, decoded) == frame_read::ok);
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
        check_eq(read_frame(iss, decoded), frame_read::message_too_big);
    };

    tester::bdd::scenario("websocket read_frame fails on truncated payload, [net]") = [] {
        // Announce 5 bytes but only provide 2.
        auto oss = std::ostringstream{};
        require_true(write_frame(oss, make_masked_text("hello"sv)));
        auto truncated = oss.str().substr(0, oss.str().size() - 3);

        auto iss = std::istringstream{truncated};
        auto decoded = frame{};
        check_true(read_frame(iss, decoded) != frame_read::ok);
    };

    tester::bdd::scenario("run_text_session echoes text via handler, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_text("hello"sv))};
        run_text_session(stream, [](std::string_view msg) {
            return text_reply{std::string{msg} + "!"};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_true(not reply.masked);
        check_eq(reply.op, opcode::text);
        check_eq(payload_as_string(reply), "hello!"s);
    };

    tester::bdd::scenario("run_text_session suppresses reply when handler returns nullopt, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_text("quiet"sv))};
        run_text_session(stream, [](std::string_view) {
            return text_reply{};
        });
        check_true(stream.output().empty());
    };

    tester::bdd::scenario("run_text_session answers ping with pong, [net]") = [] {
        auto stream = duplex_stream{frame_bytes(make_masked_ping("hb"sv))};
        run_text_session(stream, nullptr);

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
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
        run_text_session(stream, [](std::string_view msg) {
            return text_reply{std::string{msg}};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(reply.op, opcode::close);

        auto extra = frame{};
        check_true(read_frame(out, extra) != frame_read::ok); // nothing echoed after close
    };

    tester::bdd::scenario("run_text_session rejects unmasked client frame with 1002, [net]") = [] {
        // RFC 6455 §5.1: client frames must be masked. An unmasked text frame must
        // be rejected with a protocol-error close and the handler must not run.
        auto handler_ran = std::make_shared<bool>(false);
        auto stream = duplex_stream{frame_bytes(make_text_frame("nomask"sv))};
        run_text_session(stream, [handler_ran](std::string_view msg) {
            *handler_ran = true;
            return text_reply{std::string{msg}};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(reply.op, opcode::close);
        check_eq(close_code(reply), close_protocol_error);
        check_true(not *handler_ran);

        auto extra = frame{};
        check_true(read_frame(out, extra) != frame_read::ok); // session ended after the close
    };

    tester::bdd::scenario("is_valid_utf8 accepts text and rejects illegal bytes, [net]") = [] {
        check_true(is_valid_utf8("hello"sv));
        check_true(is_valid_utf8("äö€"sv)); // multi-byte OK
        check_true(is_valid_utf8(""sv));

        const auto overlong = std::array{std::byte{0xC0}, std::byte{0x80}}; // U+0000 overlong
        check_true(not is_valid_utf8(std::span<const std::byte>{overlong}));

        const auto bad = std::array{std::byte{0xFFu}};
        check_true(not is_valid_utf8(std::span<const std::byte>{bad}));

        const auto truncated = std::array{std::byte{0xE2}, std::byte{0x82}}; // incomplete €
        check_true(not is_valid_utf8(std::span<const std::byte>{truncated}));
    };

    tester::bdd::scenario("read_frame rejects RSV bits and oversized control, [net]") = [] {
        // FIN + text with RSV1 set.
        auto rsv = std::string{};
        rsv.push_back(static_cast<char>(0xC1)); // FIN|RSV1|text
        rsv.push_back(static_cast<char>(0x80)); // masked, len 0
        rsv.append("\x01\x02\x03\x04", 4);
        auto iss = std::istringstream{rsv};
        auto decoded = frame{};
        check_eq(read_frame(iss, decoded), frame_read::protocol_error);

        // Control payload must be ≤ 125: claim 126 via 16-bit length on ping.
        auto ctrl = std::string{};
        ctrl.push_back(static_cast<char>(0x89)); // FIN + ping
        ctrl.push_back(static_cast<char>(0xFE)); // masked + 126
        ctrl.push_back(static_cast<char>(0x00));
        ctrl.push_back(static_cast<char>(0x7E)); // 126 bytes
        auto iss2 = std::istringstream{ctrl};
        check_eq(read_frame(iss2, decoded), frame_read::protocol_error);
    };

    tester::bdd::scenario("run_text_session rejects fragmented text with 1003, [net]") = [] {
        auto frag = make_masked_text("part"sv);
        frag.fin = false;
        auto handler_ran = std::make_shared<bool>(false);
        auto stream = duplex_stream{frame_bytes(frag)};
        run_text_session(stream, [handler_ran](std::string_view) {
            *handler_ran = true;
            return text_reply{};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(reply.op, opcode::close);
        check_eq(close_code(reply), close_unsupported_data);
        check_true(not *handler_ran);
    };

    tester::bdd::scenario("run_text_session rejects continuation and binary with 1003, [net]") = [] {
        auto cont = frame{};
        cont.op = opcode::continuation;
        cont.fin = true;
        cont.masked = true;
        cont.masking_key = 0x01020304u;

        auto stream = duplex_stream{frame_bytes(cont)};
        run_text_session(stream, nullptr);
        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(close_code(reply), close_unsupported_data);

        auto bin = frame{};
        bin.op = opcode::binary;
        bin.masked = true;
        bin.masking_key = 0x01020304u;
        bin.payload = {std::byte{0x01}};
        auto stream2 = duplex_stream{frame_bytes(bin)};
        run_text_session(stream2, nullptr);
        auto out2 = std::istringstream{stream2.output()};
        require_true(read_frame(out2, reply) == frame_read::ok);
        check_eq(close_code(reply), close_unsupported_data);
    };

    tester::bdd::scenario("run_text_session rejects invalid UTF-8 with 1007, [net]") = [] {
        auto bad = make_masked_text("x"sv);
        bad.payload = {std::byte{0xFFu}};
        auto handler_ran = std::make_shared<bool>(false);
        auto stream = duplex_stream{frame_bytes(bad)};
        run_text_session(stream, [handler_ran](std::string_view) {
            *handler_ran = true;
            return text_reply{};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(close_code(reply), close_invalid_payload);
        check_true(not *handler_ran);
    };

    tester::bdd::scenario("run_text_session rejects oversized frame with 1009, [net]") = [] {
        // Header only: length > 1 MiB; session must close 1009 without running handler.
        auto header = std::string{};
        header.push_back(static_cast<char>(0x81)); // FIN + text
        header.push_back(static_cast<char>(0xFF)); // masked + 127
        const auto len = std::uint64_t{(1u << 20) + 1};
        for(int shift = 56; shift >= 0; shift -= 8)
            header.push_back(static_cast<char>((len >> shift) & 0xFFu));
        header.append("\x01\x02\x03\x04", 4); // masking key; no payload bytes

        auto handler_ran = std::make_shared<bool>(false);
        auto stream = duplex_stream{std::move(header)};
        run_text_session(stream, [handler_ran](std::string_view) {
            *handler_ran = true;
            return text_reply{};
        });

        auto out = std::istringstream{stream.output()};
        auto reply = frame{};
        require_true(read_frame(out, reply) == frame_read::ok);
        check_eq(close_code(reply), close_message_too_big);
        check_true(not *handler_ran);
    };

    tester::bdd::scenario("http server websocket echo upgrade, [net]") = [] {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->ws("/echo").ws([](std::string_view msg) {
            return text_reply{std::string{msg}};
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

        auto client = net::connect("127.0.0.1", "18090");
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
        require_true(read_frame(client, reply) == frame_read::ok);
        check_eq(reply.op, opcode::text);
        check_eq(payload_as_string(reply), "ping-me"s);

        require_true(write_frame(client, make_masked_close()));
        auto close_reply = frame{};
        require_true(read_frame(client, close_reply) == frame_read::ok);
        check_eq(close_reply.op, opcode::close);

        server->stop();
        if(server_thread.joinable())
            server_thread.join();
    };

    tester::bdd::scenario("websocket::connect send/recv/close echo, [net]") = [] {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->ws("/events").ws([](std::string_view msg) {
            return text_reply{std::string{msg}};
        });

        std::promise<void> bound;
        auto bound_future = bound.get_future();
        std::thread server_thread{[server, &bound] {
            try
            {
                server->listen("127.0.0.1"sv, "18091"sv, [&bound] { bound.set_value(); });
            }
            catch(...)
            {
                try { bound.set_value(); } catch(...) {}
            }
        }};

        using namespace std::chrono_literals;
        require_true(bound_future.wait_for(3s) == std::future_status::ready);
        std::this_thread::sleep_for(50ms);

        auto ws = websocket::connect("127.0.0.1"sv, "18091"sv, "/events"sv);
        require_true(static_cast<bool>(ws));
        require_true(ws.send("hello"sv));
        auto reply = ws.recv();
        require_true(reply.has_value());
        check_eq(*reply, "hello"s);
        ws.close();
        check_true(not ws);

        server->stop();
        if(server_thread.joinable())
            server_thread.join();
    };

    // Regression: close() used to drain forever waiting for a peer close frame.
    // A peer that completes the upgrade but never replies must not hang the client.
    tester::bdd::scenario("websocket::connect close times out on silent peer, [net]") = [] {
        if(not network_tests_enabled())
            return;

        using namespace std::chrono_literals;

        auto acc = net::acceptor{"127.0.0.1", "0"};
        const auto port = std::to_string(acc.bound_port());
        auto peer_ready = std::promise<void>{};
        auto peer_ready_future = peer_ready.get_future();
        auto stop_peer = std::atomic<bool>{false};

        std::thread peer_thread{[&] {
            try
            {
                peer_ready.set_value();
                auto [stream, client, client_port] = acc.accept();

                auto method = ""s;
                auto target = ""s;
                auto version = ""s;
                stream >> method >> target >> version;
                auto discard = ""s;
                std::getline(stream, discard);
                auto hs = http::headers{};
                stream >> hs >> net::crlf;
                if(not hs.contains("sec-websocket-key"))
                    return;
                const auto accept = sec_websocket_accept(hs["sec-websocket-key"]);

                stream << "HTTP/1.1 101 Switching Protocols" << net::crlf
                       << "Upgrade: websocket" << net::crlf
                       << "Connection: Upgrade" << net::crlf
                       << "Sec-WebSocket-Accept: " << accept << net::crlf
                       << net::crlf << net::flush;

                // Stay open without answering the client's close frame.
                while(not stop_peer.load())
                    std::this_thread::sleep_for(20ms);
            }
            catch(...)
            {
                try { peer_ready.set_value(); } catch(...) {}
            }
        }};

        require_true(peer_ready_future.wait_for(3s) == std::future_status::ready);

        auto ws = websocket::connect("127.0.0.1"sv, port, "/"sv);
        require_true(static_cast<bool>(ws));

        const auto started = std::chrono::steady_clock::now();
        ws.close(close_normal, 300ms);
        const auto elapsed = std::chrono::steady_clock::now() - started;

        check_true(not ws);
        // Must return near the drain budget, not hang on blocking recv.
        check_true(elapsed < 2s);
        check_true(elapsed >= 200ms);

        stop_peer = true;
        if(peer_thread.joinable())
            peer_thread.join();
    };

    // Large text frames exercise istream::read via endpointbuf::xsgetn's bulk path
    // (payload > tcp_buffer_size). A short recv must not abort the frame.
    tester::bdd::scenario("websocket::connect echoes large text payload, [net]") = [] {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->ws("/big").ws([](std::string_view msg) {
            return text_reply{std::string{msg}};
        });

        std::promise<void> bound;
        auto bound_future = bound.get_future();
        std::thread server_thread{[server, &bound] {
            try
            {
                server->listen("127.0.0.1"sv, "18093"sv, [&bound] { bound.set_value(); });
            }
            catch(...)
            {
                try { bound.set_value(); } catch(...) {}
            }
        }};

        using namespace std::chrono_literals;
        require_true(bound_future.wait_for(3s) == std::future_status::ready);
        std::this_thread::sleep_for(50ms);

        auto payload = std::string(tcp_buffer_size + 2048, 'W');
        for(std::size_t i = 0; i < payload.size(); ++i)
            payload[i] = static_cast<char>('A' + static_cast<int>(i % 26));

        auto ws = websocket::connect("127.0.0.1"sv, "18093"sv, "/big"sv);
        require_true(ws.send(payload));
        auto reply = ws.recv();
        require_true(reply.has_value());
        check_eq(*reply, payload);
        ws.close();

        server->stop();
        if(server_thread.joinable())
            server_thread.join();
    };

    tester::bdd::scenario("websocket::connect read_loop collects text, [net]") = [] {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        auto count = std::make_shared<int>(0);
        server->ws("/events").ws([count](std::string_view msg) {
            ++(*count);
            return text_reply{std::string{msg} + "-" + std::to_string(*count)};
        });

        std::promise<void> bound;
        auto bound_future = bound.get_future();
        std::thread server_thread{[server, &bound] {
            try
            {
                server->listen("127.0.0.1"sv, "18092"sv, [&bound] { bound.set_value(); });
            }
            catch(...)
            {
                try { bound.set_value(); } catch(...) {}
            }
        }};

        using namespace std::chrono_literals;
        require_true(bound_future.wait_for(3s) == std::future_status::ready);
        std::this_thread::sleep_for(50ms);

        auto ws = websocket::connect("127.0.0.1"sv, "18092"sv, "/events"sv);
        require_true(ws.send("a"sv));
        require_true(ws.send("b"sv));

        auto seen = std::vector<std::string>{};
        ws.read_loop([&](std::string_view msg) {
            seen.emplace_back(msg);
            if(seen.size() == 2u)
                ws.close();
        });
        require_true(seen.size() == 2u);
        check_eq(seen[0], "a-1"s);
        check_eq(seen[1], "b-2"s);

        server->stop();
        if(server_thread.joinable())
            server_thread.join();
    };

    return true;
}

const auto _ = register_websocket_tests();
