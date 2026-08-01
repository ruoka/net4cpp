// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import :endpointbuf;
import :endpointstream;
import :posix;
import :socket;
import :sse;
import tester;
import std;

using namespace http::sse;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

using tester::assertions::check_eq;
using tester::assertions::check_false;
using tester::assertions::check_true;
using tester::assertions::require_eq;
using tester::assertions::require_true;

auto register_sse_tests()
{
    using namespace tester::basic;

    test_case("SSE framing helpers, [net]") = []
    {
        section("format_comment with and without text") = []
        {
            check_eq(format_comment(), ":\n"s);
            check_eq(format_comment("hb"), ": hb\n"s);
        };

        section("format_event single-line data") = []
        {
            check_eq(format_event("1", "tick", "42"), "event: tick\ndata: 1\nid: 42\n\n"s);
            check_eq(format_event("hello"), "data: hello\n\n"s);
        };

        section("format_event multi-line data") = []
        {
            check_eq(
                format_event("a\nb\n", "msg"),
                "event: msg\ndata: a\ndata: b\ndata: \n\n"s);
            check_eq(format_event("a\nb"), "data: a\ndata: b\n\n"s);
        };

        section("format_event empty data and retry") = []
        {
            check_eq(format_event(""), "data: \n\n"s);
            check_eq(
                format_event("x", {}, {}, std::chrono::milliseconds{3000}),
                "data: x\nretry: 3000\n\n"s);
        };
    };

    test_case("SSE session writer, [net]") = []
    {
        section("writes events and comments with flush") = []
        {
            auto buf = std::stringbuf{};
            auto os = std::ostream{&buf};
            auto s = session{os};

            require_true(s.send_comment("ok"));
            require_true(s.send_event("tick", "1", "1"));
            require_true(s.send_data("plain"));
            check_false(s.closed());
            check_eq(buf.str(), ": ok\nevent: tick\ndata: 1\nid: 1\n\ndata: plain\n\n"s);
            check_eq(s.bytes_out(), buf.str().size());
        };

        section("closed after stream failure") = []
        {
            auto os = std::ostringstream{};
            auto s = session{os};

            require_true(s.send_comment("ok"));
            os.setstate(std::ios::badbit);
            check_true(s.closed());
            check_false(s.send_comment("more"));
            check_false(s.flush());
        };

        // Regression: stop() → endpointstream::shutdown() left failbit clear, so
        // MCP handlers that only poll session.closed() never exited and
        // wait_for_handlers() deadlocked after drain-on-listen-exit.
        section("closed after endpointstream shutdown latch") = []
        {
            auto stream = net::endpointstream{
                new net::tcp_endpointbuf{net::socket{net::posix::af_inet, net::posix::sock_stream}}};
            auto s = session{stream};
            check_false(stream.shut_down());
            check_false(s.closed());
            stream.shutdown();
            check_true(stream.shut_down());
            check_true(s.closed());
        };
    };

    return true;
}

const auto sse_test_registrar = register_sse_tests();

} // namespace
