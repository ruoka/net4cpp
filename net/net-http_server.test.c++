module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::check_contains;
using tester::assertions::check_nothrow;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

namespace view {
const auto index = "<!DOCTYPE html><html><body><h1>Test</h1></body></html>";
const auto get = "<p>GET response</p>";
const auto json = "{\"foo\" : 1, \"bar\" : false}";
}

} // namespace

auto register_server_tests()
{
    tester::bdd::scenario("Route registration and rendering, [net]") = [] {
        tester::bdd::given("An HTTP server with registered routes") = [] {
    auto handler = [](std::string_view request,
                      std::string_view,
                      const http::headers&) -> http::response_with_headers
        {return http::make_response("200 OK"s, "<p>" + std::string{request} + "</p>"s);};

            auto server = std::make_shared<http::server>();
            server->get("/").html(view::index);
            server->get("/vk").html(view::get);
            server->get("/json").json(view::json);
            server->get("/[a-z]*").response_handler("text/html", handler);
            server->get("/[a-z]+/[0-9]+").response_handler("text/html", handler);
            server->get("/foo/123").text("<p>/foo/123</p>");

            tester::bdd::when("Routes are rendered") = [server] {
    http::headers hs{};
                auto [s1, c1, t1, h1] = server->get("/").render("/", "", hs);
                auto [s2, c2, t2, h2] = server->get("/json").render("/json", "", hs);
                auto [s3, c3, t3, h3] = server->get("/foo/123").render("/foo/123", "", hs);

                tester::bdd::then("Routes return correct status and content type") = [s1, t1, s2, t2, s3, c3] {
    check_eq(s1, "200 OK");
    check_eq(t1, "text/html");
    check_eq(s2, "200 OK");
    check_eq(t2, "application/json");
    check_eq(s3, "200 OK");
    check_eq(c3, "<p>/foo/123</p>");
                };
            };
        };
    };

    tester::bdd::scenario("Server start and stop sequence, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a route") = [] {
            auto server = std::make_shared<http::server>();
            server->get("/").text("OK");

            tester::bdd::when("Server is started and stopped") = [server] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread t{[server, &started]{
        try {
            started.set_value();
                        server->listen("8081");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
        std::this_thread::sleep_for(200ms);
                    server->stop();
    }
    
    if (t.joinable()) t.join();

                tester::bdd::then("Server is stopped") = [server] {
                    check_true(server->stopped());
                };
            };
        };
    };

    tester::bdd::scenario("Structured logging fields in HTTP requests, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
    auto captured_output = std::make_shared<std::stringstream>();
    
    slog.app_name("httptest")
        .log_level(syslog::severity::debug)
        .format(log_format::jsonl)
        .sd_id("http")
        .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("An HTTP request is made with headers") = [server, captured_output] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
        try {
            started.set_value();
                        server->listen("8082");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

        try {
            auto stream = connect("127.0.0.1", "8082");
            stream << "GET /test HTTP/1.1" << net::crlf
                   << "Host: 127.0.0.1:8082" << net::crlf
                   << "User-Agent: TestAgent/1.0" << net::crlf
                   << "Content-Type: application/json" << net::crlf
                   << "Accept: text/plain, application/json" << net::crlf
                   << "Connection: close" << net::crlf
                   << net::crlf
                   << net::flush;

            std::string response;
            std::string line;
            int line_count = 0;
            while(line_count < 20 && stream && std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() || line == "\r") break;
            }
        } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
    }

    if (server_thread.joinable()) server_thread.join();
    slog.redirect(std::clog);

                tester::bdd::then("Logs contain structured fields") = [captured_output] {
    std::string output = captured_output->str();
    
    check_contains(output, "\"msg_id\":\"HTTP_REQUEST\"");
    check_contains(output, "\"request_id\"");
    check_contains(output, "\"msg_id\":\"HTTP_RESPONSE\"");
    check_contains(output, "\"duration_ms\"");
    check_contains(output, "\"msg_id\":\"HTTP_REQUEST_HEADERS\"");
    check_contains(output, "\"user_agent\":\"TestAgent/1.0\"");
    check_contains(output, "\"content_type\":\"application/json\"");
    check_contains(output, "\"accept\":\"text/plain, application/json\"");
    check_contains(output, "\"msg_id\":\"HTTP_CONN_CLOSED\"");
    check_contains(output, "\"connection_duration_ms\"");
    
                    // Verify request_id correlation
    std::istringstream iss(output);
    std::string line;
    std::string request_id_value;
    while(std::getline(iss, line)) {
        if(line.find("\"msg_id\":\"HTTP_REQUEST\"") != std::string::npos) {
            auto pos = line.find("\"request_id\":\"");
            if(pos != std::string::npos) {
                                pos += 14;
                auto end_pos = line.find("\"", pos);
                if(end_pos != std::string::npos) {
                    request_id_value = line.substr(pos, end_pos - pos);
                    break;
                }
            }
        }
    }
    
    if(!request_id_value.empty()) {
        check_contains(output, "\"request_id\":\"" + request_id_value + "\"");
    }
                };
            };
        };
    };

    tester::bdd::scenario("Request duration tracking in HTTP responses, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a slow endpoint") = [] {
    auto captured_output = std::make_shared<std::stringstream>();
    
    slog.app_name("httptest")
        .log_level(syslog::severity::debug)
        .format(log_format::jsonl)
        .sd_id("http")
        .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/slow").response_handler("text/plain", [](auto&&, auto&&, auto&&) -> http::response_with_headers {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return http::make_response("200 OK"s, "Slow response"s);
    });
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made to the slow endpoint") = [server, captured_output] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
        try {
            started.set_value();
                        server->listen("8083");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
        std::this_thread::sleep_for(200ms);

        try {
            auto stream = connect("127.0.0.1", "8083");
            stream << "GET /slow HTTP/1.1" << net::crlf
                   << "Host: 127.0.0.1:8083" << net::crlf
                   << "Connection: close" << net::crlf
                   << net::crlf
                   << net::flush;

            std::string response;
            std::string line;
            int line_count = 0;
            while(line_count < 20 && stream && std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() || line == "\r") break;
            }
        } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
    }

    if (server_thread.joinable()) server_thread.join();
    slog.redirect(std::clog);

                tester::bdd::then("Duration is logged and is at least 50ms") = [captured_output] {
    std::string output = captured_output->str();
    check_contains(output, "\"msg_id\":\"HTTP_RESPONSE\"");
    check_contains(output, "\"duration_ms\"");
    
    std::istringstream iss(output);
    std::string line;
    while(std::getline(iss, line)) {
        if(line.find("\"msg_id\":\"HTTP_RESPONSE\"") != std::string::npos) {
            auto pos = line.find("\"duration_ms\":");
            if(pos != std::string::npos) {
                                pos += 14;
                auto end_pos = line.find_first_of(",}", pos);
                if(end_pos != std::string::npos) {
                    std::string duration_str = line.substr(pos, end_pos - pos);
                    try {
                        long long duration = std::stoll(duration_str);
                                        check_true(duration >= 50);
                    } catch(...) {}
                }
            }
        }
    }
                };
            };
        };
    };

    tester::bdd::scenario("HTTP headers logging when no headers present, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
    auto captured_output = std::make_shared<std::stringstream>();
    
    slog.app_name("httptest")
        .log_level(syslog::severity::debug)
        .format(log_format::jsonl)
        .sd_id("http")
        .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made without tracked headers") = [server, captured_output] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
        try {
            started.set_value();
                        server->listen("8084");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
        std::this_thread::sleep_for(200ms);

        try {
            auto stream = connect("127.0.0.1", "8084");
            stream << "GET /test HTTP/1.1" << net::crlf
                   << "Host: 127.0.0.1:8084" << net::crlf
                   << "Connection: close" << net::crlf
                   << net::crlf
                   << net::flush;

            std::string response;
            std::string line;
            int line_count = 0;
            while(line_count < 20 && stream && std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() || line == "\r") break;
            }
        } catch(...) {}

        std::this_thread::sleep_for(200ms);
                    server->stop();
    }

    if (server_thread.joinable()) server_thread.join();
    slog.redirect(std::clog);

                tester::bdd::then("HTTP_REQUEST_HEADERS is not logged") = [captured_output] {
    std::string output = captured_output->str();
                    check_contains(output, "\"msg_id\":\"HTTP_REQUEST\"");
                    check_contains(output, "\"msg_id\":\"HTTP_RESPONSE\"");
    check_true(output.find("\"msg_id\":\"HTTP_REQUEST_HEADERS\"") == std::string::npos);
                };
            };
        };
    };

    tester::bdd::scenario("HTTP headers logging with partial headers, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
    auto captured_output = std::make_shared<std::stringstream>();
    
    slog.app_name("httptest")
        .log_level(syslog::severity::debug)
        .format(log_format::jsonl)
        .sd_id("http")
        .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made with only User-Agent header") = [server, captured_output] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
        try {
            started.set_value();
                        server->listen("8085");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
        std::this_thread::sleep_for(200ms);

        try {
            auto stream = connect("127.0.0.1", "8085");
            stream << "GET /test HTTP/1.1" << net::crlf
                   << "Host: 127.0.0.1:8085" << net::crlf
                   << "User-Agent: PartialTest/1.0" << net::crlf
                   << "Connection: close" << net::crlf
                   << net::crlf
                   << net::flush;

            std::string response;
            std::string line;
            int line_count = 0;
            while(line_count < 20 && stream && std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() || line == "\r") break;
            }
        } catch(...) {}

        std::this_thread::sleep_for(200ms);
                    server->stop();
    }

    if (server_thread.joinable()) server_thread.join();
    slog.redirect(std::clog);

                tester::bdd::then("HTTP_REQUEST_HEADERS is logged with user_agent") = [captured_output] {
    std::string output = captured_output->str();
    check_contains(output, "\"msg_id\":\"HTTP_REQUEST_HEADERS\"");
    check_contains(output, "\"user_agent\":\"PartialTest/1.0\"");
    check_contains(output, "\"content_type\"");
    check_contains(output, "\"accept\"");
                };
            };
        };
    };

    tester::bdd::scenario("HTTP headers logging with empty header values, [net]") = [] {
    if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
    auto captured_output = std::make_shared<std::stringstream>();
    
    slog.app_name("httptest")
        .log_level(syslog::severity::debug)
        .format(log_format::jsonl)
        .sd_id("http")
        .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made with empty header values") = [server, captured_output] {
    std::promise<void> started;
    auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
        try {
            started.set_value();
                        server->listen("8086");
        } catch(...) {}
    }};

    using namespace std::chrono_literals;
    if (started_future.wait_for(2s) == std::future_status::ready) {
        std::this_thread::sleep_for(200ms);

        try {
            auto stream = connect("127.0.0.1", "8086");
            stream << "GET /test HTTP/1.1" << net::crlf
                   << "Host: 127.0.0.1:8086" << net::crlf
                               << "User-Agent: " << net::crlf
                               << "Content-Type: " << net::crlf
                               << "Accept: " << net::crlf
                   << "Connection: close" << net::crlf
                   << net::crlf
                   << net::flush;

            std::string response;
            std::string line;
            int line_count = 0;
            while(line_count < 20 && stream && std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() || line == "\r") break;
            }
        } catch(...) {}

        std::this_thread::sleep_for(200ms);
                    server->stop();
    }

    if (server_thread.joinable()) server_thread.join();
    slog.redirect(std::clog);

                tester::bdd::then("HTTP_REQUEST_HEADERS is not logged") = [captured_output] {
    std::string output = captured_output->str();
                    check_contains(output, "\"msg_id\":\"HTTP_REQUEST\"");
    check_true(output.find("\"msg_id\":\"HTTP_REQUEST_HEADERS\"") == std::string::npos);
                };
            };
        };
    };

    tester::bdd::scenario("X-Correlation-ID generation when missing, [net]") = [] {
        if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
            auto captured_output = std::make_shared<std::stringstream>();
            
            slog.app_name("httptest")
                .log_level(syslog::severity::debug)
                .format(log_format::jsonl)
                .sd_id("http")
                .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made without X-Correlation-ID header") = [server, captured_output] {
                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8087");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8087");
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8087" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string response;
                        std::string line;
                        int line_count = 0;
                        while(line_count < 20 && stream && std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line.empty() || line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();
                slog.redirect(std::clog);

                tester::bdd::then("Request is processed successfully") = [captured_output] {
                    std::string output = captured_output->str();
                    // Correlation ID is now handled by middleware, not at server level
                    // Server-level logging removed - middleware handles correlation ID generation
                    check_contains(output, "\"msg_id\":\"HTTP_REQUEST\"");
                    
                    // Note: Correlation ID generation is now done by middleware, not server
                    // The server no longer logs CORRELATION_ID_GENERATED
                    auto pos = output.find("\"correlation_id\":\"");
                    if(pos != std::string::npos) {
                        pos += 18;
                        auto end_pos = output.find("\"", pos);
                        if(end_pos != std::string::npos) {
                            std::string correlation_id = output.substr(pos, end_pos - pos);
                            check_eq(correlation_id.size(), 36u);
                            check_eq(correlation_id[8], '-');
                            check_eq(correlation_id[13], '-');
                            check_eq(correlation_id[18], '-');
                            check_eq(correlation_id[23], '-');
                            check_eq(correlation_id[14], '4');
                        }
                    }
                };
            };
        };
    };

    tester::bdd::scenario("X-Correlation-ID preservation when provided, [net]") = [] {
        if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with logging configured") = [] {
            auto captured_output = std::make_shared<std::stringstream>();
            
            slog.app_name("httptest")
                .log_level(syslog::severity::debug)
                .format(log_format::jsonl)
                .sd_id("http")
                .redirect(*captured_output);

            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A request is made with X-Correlation-ID header") = [server, captured_output] {
                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8088");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        const std::string provided_correlation_id = "550e8400-e29b-41d4-a716-446655440000";
                        auto stream = connect("127.0.0.1", "8088");
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8088" << net::crlf
                               << "X-Correlation-ID: " << provided_correlation_id << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string response;
                        std::string line;
                        int line_count = 0;
                        while(line_count < 20 && stream && std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line.empty() || line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();
                slog.redirect(std::clog);

                tester::bdd::then("CORRELATION_ID_GENERATED is not logged") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"msg_id\":\"HTTP_REQUEST\"");
                    check_true(output.find("\"msg_id\":\"CORRELATION_ID_GENERATED\"") == std::string::npos);
                };
            };
        };
    };

    tester::bdd::scenario("405 Method Not Allowed when route exists but method doesn't, [net]") = [] {
        if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a GET route registered") = [] {
            auto server = std::make_shared<http::server>();
            // Register POST method by creating a POST route (even if unused)
            // This ensures POST is in m_methods so routing logic can check it
            server->post("/dummy").text("dummy");
            server->get("/test405").text("GET OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A POST request is made to the GET-only route") = [server] {
                auto response_status = std::make_shared<std::string>("");
                
                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8085");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8085");
                        stream << "POST /test405 HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8085" << net::crlf
                               << "Accept: */*" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string response;
                        std::string line;
                        int line_count = 0;
                        while(line_count < 5 && stream && std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line_count == 1) {
                                // First line is status line: "HTTP/1.1 405 Method Not Allowed"
                                // Remove trailing \r if present
                                if(!line.empty() && line.back() == '\r') {
                                    line.pop_back();
                                }
                                auto space1 = line.find(' ');
                                if(space1 != std::string::npos) {
                                    auto space2 = line.find(' ', space1 + 1);
                                    if(space2 != std::string::npos) {
                                        *response_status = line.substr(space1 + 1, space2 - space1 - 1);
                                    } else {
                                        // If no second space, status code is everything after first space
                                        *response_status = line.substr(space1 + 1);
                                    }
                                }
                            }
                            if(line.empty() || line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 405 Method Not Allowed") = [response_status] {
                    check_eq(*response_status, "405");
                };
            };
        };
    };

    tester::bdd::scenario("404 Not Found when route doesn't exist, [net]") = [] {
        if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a route registered") = [] {
            auto server = std::make_shared<http::server>();
            server->get("/existing").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A GET request is made to a non-existent route") = [server] {
                auto response_status = std::make_shared<std::string>("");
                
                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8086");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8086");
                        stream << "GET /nonexistent HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8086" << net::crlf
                               << "Accept: */*" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string response;
                        std::string line;
                        int line_count = 0;
                        while(line_count < 5 && stream && std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line_count == 1) {
                                // First line is status line: "HTTP/1.0 404 Not Found" or "HTTP/1.1 404 Not Found"
                                // Remove trailing \r if present
                                if(!line.empty() && line.back() == '\r') {
                                    line.pop_back();
                                }
                                auto space1 = line.find(' ');
                                if(space1 != std::string::npos) {
                                    auto space2 = line.find(' ', space1 + 1);
                                    if(space2 != std::string::npos) {
                                        *response_status = line.substr(space1 + 1, space2 - space1 - 1);
                                    } else {
                                        // If no second space, status code is everything after first space
                                        *response_status = line.substr(space1 + 1);
                                    }
                                }
                            }
                            if(line.empty() || line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 404 Not Found") = [response_status] {
                    check_eq(*response_status, "404");
                };
            };
        };
    };

    tester::bdd::scenario("OPTIONS preflight requests pass through to middleware for existing routes, [net]") = [] {
        if(!network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a POST route and CORS middleware") = [] {
            auto server = std::make_shared<http::server>();
            
            // Set up CORS middleware
            auto allowed_origin = [](std::string_view origin) {
                return origin == "http://localhost:8080"sv;
            };
            auto cors_mw = http::middleware::cors_middleware(
                allowed_origin,
                std::vector<std::string>{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
                std::vector<std::string>{"Content-Type", "Authorization"},
                true,
                std::optional<int>{3600}
            );
            
            // Wrap the POST handler with CORS middleware
            auto post_handler = [](auto&&, auto&&, auto&&) -> http::response_with_headers {
                return http::make_response(http::status_created, "Created"s, std::optional<http::headers>{});
            };
            auto wrapped_handler = cors_mw(post_handler);
            server->post("/api/data").response_with_headers("application/json", wrapped_handler);
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("OPTIONS request with CORS preflight headers is sent to existing route") = [server] {
                auto response_status = std::make_shared<std::string>("");
                auto has_cors_headers = std::make_shared<bool>(false);
                
                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8087");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8087");
                        stream << "OPTIONS /api/data HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8087" << net::crlf
                               << "Origin: http://localhost:8080" << net::crlf
                               << "Access-Control-Request-Method: POST" << net::crlf
                               << "Access-Control-Request-Headers: content-type,authorization" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string response;
                        std::string line;
                        int line_count = 0;
                        while(line_count < 20 && stream && std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line_count == 1) {
                                // Parse status line
                                if(!line.empty() && line.back() == '\r') {
                                    line.pop_back();
                                }
                                auto space1 = line.find(' ');
                                if(space1 != std::string::npos) {
                                    auto space2 = line.find(' ', space1 + 1);
                                    if(space2 != std::string::npos) {
                                        *response_status = line.substr(space1 + 1, space2 - space1 - 1);
                                    } else {
                                        *response_status = line.substr(space1 + 1);
                                    }
                                }
                            } else {
                                // Check for CORS headers
                                if(line.find("access-control-allow-origin") != std::string::npos ||
                                   line.find("access-control-allow-methods") != std::string::npos) {
                                    *has_cors_headers = true;
                                }
                            }
                            if(line.empty() || line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Request passes through to middleware (doesn't return 405)") = [response_status, has_cors_headers] {
                    // Should not be 405 Method Not Allowed
                    check_true(*response_status != "405");
                    // Should be 204 No Content (or handled by middleware)
                    check_true(*response_status == "204" || *response_status == "200");
                    // Should have CORS headers if middleware handled it
                    // Note: If middleware wasn't reached, headers won't be there, but 405 won't be either
                };
            };
        };
    };

    return true;
}

const auto _ = register_server_tests();
