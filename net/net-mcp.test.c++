// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import :mcp;
import tester;
import std;

using namespace http::mcp;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

using tester::assertions::check_eq;
using tester::assertions::check_false;
using tester::assertions::check_true;
using tester::assertions::check_contains;
using tester::assertions::require_true;
using tester::assertions::require_eq;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

// Parse `event:` / `data:` pairs from an SSE body (v1 subset).
inline std::vector<std::pair<std::string, std::string>> parse_sse_events(std::string_view body)
{
    auto events = std::vector<std::pair<std::string, std::string>>{};
    auto event = ""s;
    auto data = ""s;
    auto append_data = [&](std::string_view line)
    {
        if(not data.empty())
            data.push_back('\n');
        data.append(line);
    };

    while(not body.empty())
    {
        const auto nl = body.find('\n');
        auto line = nl == std::string_view::npos ? body : body.substr(0, nl);
        if(not line.empty() and line.back() == '\r')
            line.remove_suffix(1);
        if(nl == std::string_view::npos)
            body = {};
        else
            body.remove_prefix(nl + 1);

        if(line.empty())
        {
            if(not event.empty() or not data.empty())
                events.emplace_back(std::move(event), std::move(data));
            event.clear();
            data.clear();
            continue;
        }
        if(line.starts_with(":"sv))
            continue;
        if(line.starts_with("event:"sv))
        {
            auto v = line.substr(6);
            if(v.starts_with(' '))
                v.remove_prefix(1);
            event = std::string{v};
        }
        else if(line.starts_with("data:"sv))
        {
            auto v = line.substr(5);
            if(v.starts_with(' '))
                v.remove_prefix(1);
            append_data(v);
        }
    }
    return events;
}

auto register_mcp_tests()
{
    using namespace tester::basic;

    test_case("MCP query_param and session id helpers, [net]") = []
    {
        section("query_param extracts session_id") = []
        {
            const auto sid = query_param("/messages/?session_id=abc123&x=1", "session_id");
            require_true(sid.has_value());
            check_eq(*sid, "abc123"s);
            check_false(query_param("/messages/", "session_id").has_value());
            check_false(query_param("/messages/?foo=1", "session_id").has_value());
        };

        section("make_session_id is 32 hex chars") = []
        {
            const auto id = make_session_id();
            check_eq(id.size(), 32u);
            check_true(std::ranges::all_of(id, [](char c) {
                return (c >= '0' and c <= '9') or (c >= 'a' and c <= 'f');
            }));
        };
    };

    test_case("MCP SSE transport session + POST path, [net]") = []
    {
        if(not network_tests_enabled())
            return;

        section("endpoint event, POST 202, message echo, unknown session 404") = []
        {
            using namespace std::chrono_literals;

            auto server = std::make_shared<http::server>();
            auto transport = std::make_shared<sse_transport>("/messages/");
            auto got_post = std::make_shared<std::string>();

            transport->attach(*server, "/sse", [got_post](session& sess) {
                if(auto msg = sess.recv(3s))
                {
                    *got_post = *msg;
                    sess.send_message(*msg);
                }
            });
            server->timeout(std::chrono::seconds{1});

            std::promise<void> started;
            auto started_future = started.get_future();
            std::thread server_thread{[server, &started] {
                try
                {
                    started.set_value();
                    server->listen("8101");
                }
                catch(...)
                {
                }
            }};

            auto endpoint_data = ""s;
            auto message_data = ""s;
            auto post_status = ""s;
            auto missing_status = ""s;
            auto posted = false;

            if(started_future.wait_for(2s) == std::future_status::ready)
            {
                std::this_thread::sleep_for(200ms);
                try
                {
                    auto stream = net::connect("127.0.0.1", "8101");
                    stream << "GET /sse HTTP/1.1" << net::crlf
                           << "Host: 127.0.0.1:8101" << net::crlf
                           << "Accept: text/event-stream" << net::crlf
                           << "Connection: close" << net::crlf
                           << net::crlf
                           << net::flush;

                    // Skip response headers.
                    std::string line;
                    while(stream and std::getline(stream, line))
                    {
                        if(not line.empty() and line.back() == '\r')
                            line.pop_back();
                        if(line.empty())
                            break;
                    }

                    auto body = ""s;
                    char ch{};
                    while(stream.get(ch))
                    {
                        body.push_back(ch);
                        auto events = parse_sse_events(body);
                        for(const auto& [ev, data] : events)
                        {
                            if(ev == "endpoint" and endpoint_data.empty())
                                endpoint_data = data;
                            if(ev == "message" and message_data.empty())
                                message_data = data;
                        }
                        if(not posted and not endpoint_data.empty())
                        {
                            posted = true;
                            const auto q = endpoint_data.find("session_id=");
                            if(q == std::string::npos)
                                break;
                            const auto sid = endpoint_data.substr(q + "session_id="sv.size());
                            auto post = net::connect("127.0.0.1", "8101");
                            const auto payload = R"({"jsonrpc":"2.0","id":1,"method":"ping"})"s;
                            post << "POST /messages/?session_id=" << sid << " HTTP/1.1" << net::crlf
                                 << "Host: 127.0.0.1:8101" << net::crlf
                                 << "Content-Type: application/json" << net::crlf
                                 << "Content-Length: " << payload.size() << net::crlf
                                 << "Connection: close" << net::crlf
                                 << net::crlf
                                 << payload
                                 << net::flush;
                            if(std::getline(post, line))
                            {
                                if(not line.empty() and line.back() == '\r')
                                    line.pop_back();
                                post_status = line;
                            }

                            // Unknown session → 404
                            auto bad = net::connect("127.0.0.1", "8101");
                            bad << "POST /messages/?session_id=deadbeefdeadbeefdeadbeefdeadbeef HTTP/1.1" << net::crlf
                                << "Host: 127.0.0.1:8101" << net::crlf
                                << "Content-Type: application/json" << net::crlf
                                << "Content-Length: 2" << net::crlf
                                << "Connection: close" << net::crlf
                                << net::crlf
                                << "{}"
                                << net::flush;
                            if(std::getline(bad, line))
                            {
                                if(not line.empty() and line.back() == '\r')
                                    line.pop_back();
                                missing_status = line;
                            }
                        }
                        if(not message_data.empty())
                            break;
                        if(body.size() > 8192)
                            break;
                    }
                }
                catch(...)
                {
                }

                std::this_thread::sleep_for(200ms);
                server->stop();
            }

            if(server_thread.joinable())
                server_thread.join();

            check_contains(endpoint_data, "/messages/?session_id=");
            check_eq(endpoint_data.find('&'), std::string::npos);
            check_contains(post_status, "202");
            check_contains(missing_status, "404");
            check_eq(*got_post, R"({"jsonrpc":"2.0","id":1,"method":"ping"})"s);
            check_eq(message_data, *got_post);
            check_eq(transport->active_sessions(), 0u);
        };
    };

    return true;
}

const auto mcp_test_registrar = register_mcp_tests();

} // namespace
