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

    test_case("MCP JSON-RPC dispatch, [net]") = []
    {
        const auto info = server_info{.name = "test"s, .version = "0.1.0"s};
        const auto list = [] {
            return std::vector<tool_spec>{
                {.name = "echo"s, .description = "Echo text"s},
            };
        };
        const auto call = [](std::string_view name, std::string_view args) -> tool_result {
            check_eq(name, "echo"sv);
            return {.text = std::string{args}, .is_error = false};
        };

        section("initialize / ping / notifications") = [info, list, call]
        {
            const auto init = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})",
                info,
                list,
                call);
            require_true(init.has_value());
            check_contains(*init, R"("protocolVersion":"2024-11-05")");
            check_contains(*init, R"("name":"test")");
            check_contains(*init, R"("capabilities":{"tools":{}})");

            check_false(handle_json_rpc(
                R"({"jsonrpc":"2.0","method":"notifications/initialized"})",
                info,
                list,
                call).has_value());

            const auto pong = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":2,"method":"ping"})",
                info,
                list,
                call);
            require_true(pong.has_value());
            check_eq(*pong, R"({"jsonrpc":"2.0","id":2,"result":{}})"s);
        };

        section("tools/list and tools/call") = [info, list, call]
        {
            const auto listed = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":3,"method":"tools/list"})",
                info,
                list,
                call);
            require_true(listed.has_value());
            check_contains(*listed, R"("name":"echo")");
            check_contains(*listed, R"("description":"Echo text")");

            const auto called = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"echo","arguments":{"x":1}}})",
                info,
                list,
                call);
            require_true(called.has_value());
            check_contains(*called, R"("text":"{\"x\":1}")");
            check_false(called->contains("isError"));
        };

        section("unknown method and unknown tool") = [info, list, call]
        {
            const auto unknown_method = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":5,"method":"nope"})",
                info,
                list,
                call);
            require_true(unknown_method.has_value());
            check_contains(*unknown_method, R"("code":-32601)");

            const auto unknown_tool = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"missing","arguments":{}}})",
                info,
                list,
                call);
            require_true(unknown_tool.has_value());
            check_contains(*unknown_tool, R"("code":-32601)");
            check_contains(*unknown_tool, "Unknown tool: missing");
        };

        section("tool error becomes isError content") = [info, list]
        {
            const auto call_err = [](std::string_view, std::string_view) -> tool_result {
                return {.text = "boom"s, .is_error = true};
            };
            const auto err = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"echo","arguments":{}}})",
                info,
                list,
                call_err);
            require_true(err.has_value());
            check_contains(*err, R"("isError":true)");
            check_contains(*err, R"("text":"boom")");
        };

        section("tools/call ignores nested arguments.name (key-order shadowing)") = [info]
        {
            // Alphabetical / BTreeMap clients emit arguments before name. A nested
            // "name" field must not select the tool (or invoke a different tool).
            auto called = std::make_shared<std::string>();
            const auto list_both = [] {
                return std::vector<tool_spec>{
                    {.name = "echo"s, .description = "Echo"s},
                    {.name = "delete_all"s, .description = "Dangerous"s},
                };
            };
            const auto call = [called](std::string_view name, std::string_view args) -> tool_result {
                *called = std::string{name} + ":"s + std::string{args};
                return {.text = "ok"s};
            };

            const auto reply = handle_json_rpc(
                R"({"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"arguments":{"name":"delete_all"},"name":"echo"}})",
                info,
                list_both,
                call);
            require_true(reply.has_value());
            check_eq(*called, R"(echo:{"name":"delete_all"})"s);
            check_contains(*reply, R"("text":"ok")");
            check_false(reply->contains("Unknown tool"sv));
        };

        section("Host/Origin allow patterns") = []
        {
            check_true(match_allow_pattern("127.0.0.1:8101", "127.0.0.1:*"));
            check_true(match_allow_pattern("localhost", "localhost:*"));
            check_false(match_allow_pattern("evil.example", "127.0.0.1:*"));
            check_true(match_allow_pattern("http://localhost:3000", "http://localhost:*"));
        };
    };

    test_case("MCP protocol server tears down session on SSE disconnect, [net]") = []
    {
        if(not network_tests_enabled())
            return;

        section("client close without POST releases session within heartbeat") = []
        {
            using namespace std::chrono_literals;

            auto http = std::make_shared<http::server>();
            auto mcp = std::make_shared<server>("/messages/");
            mcp->attach(*http, "/sse");
            http->timeout(std::chrono::seconds{1});

            std::promise<void> started;
            auto started_future = started.get_future();
            std::thread server_thread{[http, &started] {
                try
                {
                    started.set_value();
                    http->listen("8103");
                }
                catch(...)
                {
                }
            }};

            auto saw_endpoint = false;
            if(started_future.wait_for(2s) == std::future_status::ready)
            {
                std::this_thread::sleep_for(200ms);
                try
                {
                    auto stream = net::connect("127.0.0.1", "8103");
                    stream << "GET /sse HTTP/1.1" << net::crlf
                           << "Host: 127.0.0.1:8103" << net::crlf
                           << "Accept: text/event-stream" << net::crlf
                           << "Connection: close" << net::crlf
                           << net::crlf
                           << net::flush;

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
                        for(const auto& [ev, data] : parse_sse_events(body))
                        {
                            if(ev == "endpoint")
                            {
                                saw_endpoint = true;
                                break;
                            }
                        }
                        if(saw_endpoint)
                            break;
                        if(body.size() > 4096)
                            break;
                    }
                    stream.close();
                }
                catch(...)
                {
                }

                // Heartbeat is 1s; allow a few intervals for write-fail + erase.
                for(auto i = 0; i < 50 and mcp->transport().active_sessions() != 0; ++i)
                    std::this_thread::sleep_for(100ms);
                http->stop();
            }

            if(server_thread.joinable())
                server_thread.join();

            check_true(saw_endpoint);
            check_eq(mcp->transport().active_sessions(), 0u);
        };
    };

    test_case("MCP protocol server initialize and tools/list over SSE, [net]") = []
    {
        if(not network_tests_enabled())
            return;

        section("happy path + evil Origin rejected") = []
        {
            using namespace std::chrono_literals;

            auto http = std::make_shared<http::server>();
            auto mcp = std::make_shared<server>("/messages/");
            mcp->info({.name = "demo"s, .version = "1.0.0"s})
                .list_tools([] {
                    return std::vector<tool_spec>{{.name = "health"s, .description = "ok"s}};
                })
                .call_tool([](std::string_view, std::string_view) {
                    return tool_result{.text = "ok"s};
                });
            mcp->attach(*http, "/sse");
            http->timeout(std::chrono::seconds{1});

            std::promise<void> started;
            auto started_future = started.get_future();
            std::thread server_thread{[http, &started] {
                try
                {
                    started.set_value();
                    http->listen("8102");
                }
                catch(...)
                {
                }
            }};

            auto init_reply = ""s;
            auto list_reply = ""s;
            auto evil_status = ""s;

            if(started_future.wait_for(2s) == std::future_status::ready)
            {
                std::this_thread::sleep_for(200ms);
                try
                {
                    // Evil Origin on SSE open → 403 before stream.
                    {
                        auto evil = net::connect("127.0.0.1", "8102");
                        evil << "GET /sse HTTP/1.1" << net::crlf
                             << "Host: 127.0.0.1:8102" << net::crlf
                             << "Origin: http://evil.example" << net::crlf
                             << "Connection: close" << net::crlf
                             << net::crlf
                             << net::flush;
                        std::string line;
                        if(std::getline(evil, line))
                        {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            evil_status = line;
                        }
                    }

                    auto stream = net::connect("127.0.0.1", "8102");
                    stream << "GET /sse HTTP/1.1" << net::crlf
                           << "Host: 127.0.0.1:8102" << net::crlf
                           << "Origin: http://127.0.0.1:3000" << net::crlf
                           << "Accept: text/event-stream" << net::crlf
                           << "Connection: close" << net::crlf
                           << net::crlf
                           << net::flush;

                    std::string line;
                    while(stream and std::getline(stream, line))
                    {
                        if(not line.empty() and line.back() == '\r')
                            line.pop_back();
                        if(line.empty())
                            break;
                    }

                    auto body = ""s;
                    auto endpoint = ""s;
                    auto posted = false;
                    char ch{};
                    while(stream.get(ch))
                    {
                        body.push_back(ch);
                        for(const auto& [ev, data] : parse_sse_events(body))
                        {
                            if(ev == "endpoint" and endpoint.empty())
                                endpoint = data;
                            if(ev == "message")
                            {
                                if(init_reply.empty()
                                   and data.contains("protocolVersion"sv)
                                   and data.contains("serverInfo"sv))
                                    init_reply = data;
                                else if(list_reply.empty() and data.contains(R"("tools":[)"sv))
                                    list_reply = data;
                            }
                        }
                        if(not posted and not endpoint.empty())
                        {
                            posted = true;
                            const auto q = endpoint.find("session_id=");
                            if(q == std::string::npos)
                                break;
                            const auto sid = endpoint.substr(q + "session_id="sv.size());
                            auto post_init = [&](std::string_view payload) {
                                auto post = net::connect("127.0.0.1", "8102");
                                post << "POST /messages/?session_id=" << sid << " HTTP/1.1" << net::crlf
                                     << "Host: 127.0.0.1:8102" << net::crlf
                                     << "Origin: http://127.0.0.1:3000" << net::crlf
                                     << "Content-Type: application/json" << net::crlf
                                     << "Content-Length: " << payload.size() << net::crlf
                                     << "Connection: close" << net::crlf
                                     << net::crlf
                                     << payload
                                     << net::flush;
                            };
                            post_init(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"t","version":"1"}}})");
                            post_init(R"({"jsonrpc":"2.0","method":"notifications/initialized"})");
                            post_init(R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})");
                        }
                        if(not init_reply.empty() and not list_reply.empty())
                            break;
                        if(body.size() > 16384)
                            break;
                    }
                }
                catch(...)
                {
                }

                std::this_thread::sleep_for(200ms);
                http->stop();
            }

            if(server_thread.joinable())
                server_thread.join();

            check_contains(evil_status, "403");
            check_contains(init_reply, R"("name":"demo")");
            check_contains(init_reply, R"("protocolVersion":"2024-11-05")");
            check_contains(list_reply, R"("name":"health")");
        };
    };

    test_case("MCP authorize gate rejects unauthenticated SSE open, [net]") = []
    {
        if(not network_tests_enabled())
            return;

        section("missing Authorization returns 401; valid Bearer proceeds") = []
        {
            using namespace std::chrono_literals;

            auto http = std::make_shared<http::server>();
            auto mcp = std::make_shared<server>("/messages/");
            mcp->authorize([](::http::request_view, ::http::headers& hdr)
                -> std::optional<::http::response_with_headers>
            {
                std::string_view authorization{};
                if(hdr.contains("authorization"s))
                    authorization = hdr["authorization"s];
                if(authorization != "Bearer secret"sv)
                    return ::http::make_error_response(::http::status_unauthorized, "Unauthorized"sv);
                return std::nullopt;
            });
            mcp->attach(*http, "/sse");
            http->timeout(std::chrono::seconds{1});

            std::promise<void> started;
            auto started_future = started.get_future();
            std::thread server_thread{[http, &started] {
                try
                {
                    started.set_value();
                    http->listen("8104");
                }
                catch(...)
                {
                }
            }};

            auto denied_status = ""s;
            auto allowed_status = ""s;
            auto saw_endpoint = false;

            if(started_future.wait_for(2s) == std::future_status::ready)
            {
                std::this_thread::sleep_for(200ms);
                try
                {
                    {
                        auto denied = net::connect("127.0.0.1", "8104");
                        denied << "GET /sse HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8104" << net::crlf
                               << "Accept: text/event-stream" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;
                        std::string line;
                        if(std::getline(denied, line))
                        {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            denied_status = line;
                        }
                    }

                    auto stream = net::connect("127.0.0.1", "8104");
                    stream << "GET /sse HTTP/1.1" << net::crlf
                           << "Host: 127.0.0.1:8104" << net::crlf
                           << "Authorization: Bearer secret" << net::crlf
                           << "Accept: text/event-stream" << net::crlf
                           << "Connection: close" << net::crlf
                           << net::crlf
                           << net::flush;

                    std::string line;
                    if(std::getline(stream, line))
                    {
                        if(not line.empty() and line.back() == '\r')
                            line.pop_back();
                        allowed_status = line;
                    }
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
                        for(const auto& [ev, data] : parse_sse_events(body))
                        {
                            if(ev == "endpoint")
                            {
                                saw_endpoint = true;
                                break;
                            }
                        }
                        if(saw_endpoint or body.size() > 4096)
                            break;
                    }
                }
                catch(...)
                {
                }

                std::this_thread::sleep_for(200ms);
                http->stop();
            }

            if(server_thread.joinable())
                server_thread.join();

            check_contains(denied_status, "401");
            check_contains(allowed_status, "200");
            check_true(saw_endpoint);
        };
    };

    return true;
}

const auto mcp_test_registrar = register_mcp_tests();

} // namespace
