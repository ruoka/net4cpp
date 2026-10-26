// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace net;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{
using tester::assertions::check_eq;
using tester::assertions::check_false;
using tester::assertions::check_true;
using tester::basic::test_case;

const auto known = "0af7651916cd43dd8448eb211c80319c"s;
const auto span = "b7ad6b7169203331"s;
const auto header = "00-" + known + "-" + span + "-01";

bool valid_header(std::string_view text)
{
    return trace::parse_traceparent(text).has_value();
}

} // namespace

auto register_trace_tests()
{
    test_case("traceparent parses, continues, and rejects a bad header, [net]") = []
    {
        const auto parsed = trace::parse_traceparent(header);
        check_true(parsed.has_value());
        check_eq(parsed->trace_id, known);
        check_eq(parsed->span_id, span);

        const auto continued = trace::adopt(header, "vendor=ok"sv);
        check_eq(continued.trace_id, known);
        check_true(continued.span_id != span);
        check_eq(continued.span_id.size(), 16uz);
        check_eq(continued.tracestate, "vendor=ok"s);
        check_true(trace::format_traceparent(continued).starts_with("00-" + known + "-"));
        check_true(trace::format_traceparent(continued).ends_with("-01"sv));

        check_false(valid_header(""));
        check_false(valid_header("not-a-trace"));
        check_false(valid_header("00-" + known + "-" + span + "-01-extra"));
        check_false(valid_header("00-" + std::string(32, '0') + "-" + span + "-01"));
        check_false(valid_header("00-" + known + "-" + std::string(16, '0') + "-01"));
        check_false(valid_header("00-" + std::string(32, 'A') + "-" + span + "-01"));

        const auto fresh = trace::adopt("not-a-trace"sv, "vendor=drop"sv);
        check_true(fresh.trace_id != known);
        check_eq(fresh.trace_id.size(), 32uz);
        check_true(fresh.tracestate.empty());

        // A later version may carry fields after the flags.
        const auto newer = trace::parse_traceparent("01-" + known + "-" + span + "-01-extra");
        check_true(newer.has_value());
        check_eq(newer->trace_id, known);
    };

    test_case("a trace scope is on the slog line and restores the previous one, [net]") = []
    {
        auto captured = std::stringstream{};
        slog.app_name("trace")
            .log_level(syslog::severity::debug)
            .format(log_format::jsonl)
            .redirect(captured);

        const auto outer = trace::resume(known);
        {
            const auto scope = trace::scope{outer};
            slog << net::info("TRACE_OUTER") << "outer" << net::flush;
            {
                const auto inner = trace::scope{trace::resume(known)};
                slog << net::info("TRACE_INNER") << "inner" << net::flush;
            }
            slog << net::info("TRACE_BACK") << "back" << net::flush;
        }
        slog << net::info("TRACE_NONE") << "none" << net::flush;
        slog.redirect(std::clog);

        auto outer_span = ""s;
        auto inner_span = ""s;
        auto back_span = ""s;
        auto saw_none = false;
        auto line = std::string{};
        while(std::getline(captured, line))
        {
            if(line.contains("TRACE_OUTER"sv))
            {
                check_true(line.contains("\"trace_id\":\"" + known + "\""));
                const auto key = "\"span_id\":\""sv;
                const auto at = line.find(key);
                check_true(at != std::string::npos);
                outer_span = line.substr(at + key.size(), 16);
            }
            else if(line.contains("TRACE_INNER"sv))
            {
                check_true(line.contains("\"trace_id\":\"" + known + "\""));
                const auto key = "\"span_id\":\""sv;
                inner_span = line.substr(line.find(key) + key.size(), 16);
            }
            else if(line.contains("TRACE_BACK"sv))
            {
                const auto key = "\"span_id\":\""sv;
                back_span = line.substr(line.find(key) + key.size(), 16);
            }
            else if(line.contains("TRACE_NONE"sv))
            {
                saw_none = true;
                check_true(line.find("trace_id") == std::string::npos);
            }
        }
        check_true(saw_none);
        check_eq(outer_span, outer.span_id);
        check_true(not inner_span.empty() and inner_span != outer_span);
        check_eq(back_span, outer_span);
    };

    return 0;
}

const auto _ = register_trace_tests();
