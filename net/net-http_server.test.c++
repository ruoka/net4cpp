module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::check_false;
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
    if(not network_tests_enabled()) return;

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
    if(not network_tests_enabled()) return;

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
            while(line_count < 20 and stream and std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() or line == "\r") break;
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
    
    if(not request_id_value.empty()) {
        check_contains(output, "\"request_id\":\"" + request_id_value + "\"");
    }
                };
            };
        };
    };

    tester::bdd::scenario("Request duration tracking in HTTP responses, [net]") = [] {
    if(not network_tests_enabled()) return;

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
            while(line_count < 20 and stream and std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() or line == "\r") break;
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
    if(not network_tests_enabled()) return;

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
            while(line_count < 20 and stream and std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() or line == "\r") break;
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
    if(not network_tests_enabled()) return;

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
            while(line_count < 20 and stream and std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() or line == "\r") break;
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
    if(not network_tests_enabled()) return;

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
            while(line_count < 20 and stream and std::getline(stream, line)) {
                response += line + "\n";
                line_count++;
                if(line.empty() or line == "\r") break;
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
        if(not network_tests_enabled()) return;

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
                        while(line_count < 20 and stream and std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line.empty() or line == "\r") break;
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
        if(not network_tests_enabled()) return;

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
                        while(line_count < 20 and stream and std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line.empty() or line == "\r") break;
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
        if(not network_tests_enabled()) return;

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
                        while(line_count < 5 and stream and std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line_count == 1) {
                                // First line is status line: "HTTP/1.1 405 Method Not Allowed"
                                // Remove trailing \r if present
                                if(not line.empty() and line.back() == '\r') {
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
                            if(line.empty() or line == "\r") break;
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
        if(not network_tests_enabled()) return;

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
                        while(line_count < 5 and stream and std::getline(stream, line)) {
                            response += line + "\n";
                            line_count++;
                            if(line_count == 1) {
                                // First line is status line: "HTTP/1.0 404 Not Found" or "HTTP/1.1 404 Not Found"
                                // Remove trailing \r if present
                                if(not line.empty() and line.back() == '\r') {
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
                            if(line.empty() or line == "\r") break;
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

    tester::bdd::scenario("400 Bad Request when Host header is missing, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a route registered") = [] {
            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A GET request is made without a Host header") = [server] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8089");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8089");
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 400 Bad Request") = [response_status] {
                    check_eq(*response_status, "400");
                };
            };
        };
    };

    tester::bdd::scenario("400 Bad Request when Content-Length is invalid, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a route registered") = [] {
            auto server = std::make_shared<http::server>();
            server->post("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A POST request has a non-numeric Content-Length") = [server] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8090");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8090");
                        stream << "POST /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8090" << net::crlf
                               << "Content-Length: not-a-number" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 400 Bad Request") = [response_status] {
                    check_eq(*response_status, "400");
                };
            };
        };
    };

    // Regression: request-line / header parsing used unbounded operator>> and
    // getline, so a giant header (or URI) could OOM the process before the
    // Content-Length body cap applied. Cap the head and answer 431.
    tester::bdd::scenario("431 when request head exceeds max_request_head_size, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a small request-head limit") = [] {
            auto server = std::make_shared<http::server>();
            server->max_request_head_size(256);
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A GET sends a header larger than the head limit") = [server] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8095");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8095");
                        const auto filler = std::string(512, 'A');
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8095" << net::crlf
                               << "X-Fill: " << filler << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 431 Request Header Fields Too Large") = [response_status] {
                    check_eq(*response_status, "431");
                };
            };
        };
    };

    tester::bdd::scenario("413 when Content-Length exceeds max_request_body_size before allocate, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a 64-byte body limit") = [] {
            auto server = std::make_shared<http::server>();
            server->max_request_body_size(64);
            server->post("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A POST declares Content-Length above the limit without sending a body") = [server] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8091");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8091");
                        stream << "POST /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8091" << net::crlf
                               << "Content-Length: 1048576" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response status is 413 Payload Too Large") = [response_status] {
                    check_eq(*response_status, "413");
                };
            };
        };
    };

    tester::bdd::scenario("Connection header is echoed on successful responses, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a route registered") = [] {
            auto server = std::make_shared<http::server>();
            server->get("/test").text("OK");
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A GET request asks to close the connection") = [server] {
                auto response_headers = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8091");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8091");
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8091" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        int line_count = 0;
                        while(line_count < 20 and stream and std::getline(stream, line)) {
                            *response_headers += line + "\n";
                            line_count++;
                            if(line.empty() or line == "\r") break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response includes Connection: close") = [response_headers] {
                    check_contains(*response_headers, "Connection: close");
                };
            };
        };
    };

    // Regression: duplicate Content-Length used to last-wins, so a proxy that
    // honored the first value could desync and smuggle a follow-on request.
    tester::bdd::scenario("400 when Content-Length is duplicated, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a POST route") = [] {
            auto server = std::make_shared<http::server>();
            auto handled = std::make_shared<std::atomic<bool>>(false);
            server->post("/test").response_handler("text/plain",
                [handled](std::string_view, std::string_view, http::headers&) {
                    handled->store(true);
                    return http::make_response(http::status_ok, "OK"s);
                });
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A POST sends two Content-Length headers") = [server, handled] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8092");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8092");
                        stream << "POST /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8092" << net::crlf
                               << "Content-Length: 4" << net::crlf
                               << "Content-Length: 0" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << "smuggle" << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response is 400 and the handler is not invoked") = [response_status, handled] {
                    check_eq(*response_status, "400");
                    check_false(handled->load());
                };
            };
        };
    };

    // Regression: duplicate Host used to last-wins, so a proxy that honored the
    // first value could route to the victim while the app saw the attacker Host.
    tester::bdd::scenario("400 when Host is duplicated, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a GET route") = [] {
            auto server = std::make_shared<http::server>();
            auto handled = std::make_shared<std::atomic<bool>>(false);
            server->get("/test").response_handler("text/plain",
                [handled](std::string_view, std::string_view, http::headers&) {
                    handled->store(true);
                    return http::make_response(http::status_ok, "OK"s);
                });
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A GET sends two Host headers") = [server, handled] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8094");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8094");
                        stream << "GET /test HTTP/1.1" << net::crlf
                               << "Host: victim.example" << net::crlf
                               << "Host: evil.attacker" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response is 400 and the handler is not invoked") = [response_status, handled] {
                    check_eq(*response_status, "400");
                    check_false(handled->load());
                };
            };
        };
    };

    // Regression: Transfer-Encoding was ignored while Content-Length framed the
    // body, which desynchronizes from proxies that decode chunked requests.
    tester::bdd::scenario("400 when Transfer-Encoding is present, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a POST route") = [] {
            auto server = std::make_shared<http::server>();
            auto handled = std::make_shared<std::atomic<bool>>(false);
            server->post("/test").response_handler("text/plain",
                [handled](std::string_view, std::string_view, http::headers&) {
                    handled->store(true);
                    return http::make_response(http::status_ok, "OK"s);
                });
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("A POST declares Transfer-Encoding: chunked") = [server, handled] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8093");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8093");
                        stream << "POST /test HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8093" << net::crlf
                               << "Transfer-Encoding: chunked" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << "0" << net::crlf
                               << net::crlf << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response is 400 and the handler is not invoked") = [response_status, handled] {
                    check_eq(*response_status, "400");
                    check_false(handled->load());
                };
            };
        };
    };

    // Regression: OPTIONS preflight must use a registered OPTIONS handler
    // (typically cors_middleware around a no-op), never GET/POST fallbacks.
    tester::bdd::scenario("OPTIONS CORS preflight reaches cors_middleware on existing routes, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with POST plus OPTIONS wrapped in cors_middleware") = [] {
            auto server = std::make_shared<http::server>();
            auto handler_invoked = std::make_shared<std::atomic<bool>>(false);

            auto allowed_origin = [](std::string_view origin) {
                return origin == "http://localhost:8080"sv;
            };
            auto cors_mw = http::middleware::cors_middleware(
                allowed_origin,
                std::vector<std::string>{"GET"s, "POST"s, "OPTIONS"s},
                std::vector<std::string>{"Content-Type"s, "Authorization"s},
                true,
                std::optional<int>{3600});

            auto post_handler = [handler_invoked](auto&&, auto&&, auto&&) -> http::response_with_headers {
                handler_invoked->store(true);
                return http::make_response(http::status_created, "Created"s, std::optional<http::headers>{});
            };
            auto options_handler = [](auto&&, auto&&, auto&&) -> http::response_with_headers {
                return http::make_response(http::status_no_content, ""s, std::optional<http::headers>{});
            };
            server->post("/api/data").response_with_headers("application/json", cors_mw(post_handler));
            server->options("/api/data").response_with_headers("application/json", cors_mw(options_handler));
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("Browser-style OPTIONS preflight is sent to the POST route") = [server, handler_invoked] {
                auto response_status = std::make_shared<std::string>("");
                auto acao = std::make_shared<std::string>("");
                auto acam = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8094");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        auto stream = connect("127.0.0.1", "8094");
                        stream << "OPTIONS /api/data HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8094" << net::crlf
                               << "Origin: http://localhost:8080" << net::crlf
                               << "Access-Control-Request-Method: POST" << net::crlf
                               << "Access-Control-Request-Headers: content-type,authorization" << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << net::flush;

                        std::string line;
                        int line_count = 0;
                        while(line_count < 30 and stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            ++line_count;
                            if(line_count == 1) {
                                auto space1 = line.find(' ');
                                if(space1 != std::string::npos) {
                                    auto space2 = line.find(' ', space1 + 1);
                                    *response_status = space2 != std::string::npos
                                        ? line.substr(space1 + 1, space2 - space1 - 1)
                                        : line.substr(space1 + 1);
                                }
                            } else if(line.starts_with("access-control-allow-origin:"sv)) {
                                *acao = std::string{utils::trim(std::string_view{line}.substr(line.find(':') + 1))};
                            } else if(line.starts_with("access-control-allow-methods:"sv)) {
                                *acam = std::string{utils::trim(std::string_view{line}.substr(line.find(':') + 1))};
                            }
                            if(line.empty()) break;
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response is 204 with CORS headers and POST handler is not run") =
                    [response_status, acao, acam, handler_invoked] {
                        check_eq(*response_status, "204");
                        check_eq(*acao, "http://localhost:8080");
                        check_true(acam->contains("POST"));
                        check_false(handler_invoked->load());
                    };
            };
        };
    };

    // Regression: prior OPTIONS preflight fallback invoked POST when cors_middleware
    // was absent, so a preflight body could create resources.
    tester::bdd::scenario("OPTIONS preflight does not invoke POST without cors_middleware, [net]") = [] {
        if(not network_tests_enabled()) return;

        tester::bdd::given("An HTTP server with a bare POST route and no OPTIONS handler") = [] {
            auto server = std::make_shared<http::server>();
            auto handler_invoked = std::make_shared<std::atomic<bool>>(false);

            auto post_handler = [handler_invoked](auto&&, auto&&, auto&&) -> http::response_with_headers {
                handler_invoked->store(true);
                return http::make_response(http::status_created, "Created"s, std::optional<http::headers>{});
            };
            server->post("/api/data").response_with_headers("application/json", post_handler);
            server->timeout(std::chrono::seconds{1});

            tester::bdd::when("OPTIONS preflight with a body is sent to the POST path") = [server, handler_invoked] {
                auto response_status = std::make_shared<std::string>("");

                std::promise<void> started;
                auto started_future = started.get_future();

                std::thread server_thread{[server, &started]{
                    try {
                        started.set_value();
                        server->listen("8095");
                    } catch(...) {}
                }};

                using namespace std::chrono_literals;
                if (started_future.wait_for(2s) == std::future_status::ready) {
                    std::this_thread::sleep_for(200ms);

                    try {
                        const auto body = R"({"name":"preflight-must-not-create"})"s;
                        auto stream = connect("127.0.0.1", "8095");
                        stream << "OPTIONS /api/data HTTP/1.1" << net::crlf
                               << "Host: 127.0.0.1:8095" << net::crlf
                               << "Access-Control-Request-Method: POST" << net::crlf
                               << "Content-Type: application/json" << net::crlf
                               << "Content-Length: " << body.size() << net::crlf
                               << "Connection: close" << net::crlf
                               << net::crlf
                               << body
                               << net::flush;

                        std::string line;
                        if(stream and std::getline(stream, line)) {
                            if(not line.empty() and line.back() == '\r')
                                line.pop_back();
                            auto space1 = line.find(' ');
                            if(space1 != std::string::npos) {
                                auto space2 = line.find(' ', space1 + 1);
                                *response_status = space2 != std::string::npos
                                    ? line.substr(space1 + 1, space2 - space1 - 1)
                                    : line.substr(space1 + 1);
                            }
                        }
                    } catch(...) {}

                    std::this_thread::sleep_for(200ms);
                    server->stop();
                }

                if (server_thread.joinable()) server_thread.join();

                tester::bdd::then("Response is 204 and POST handler is not run") =
                    [response_status, handler_invoked] {
                        check_eq(*response_status, "204");
                        check_false(handler_invoked->load());
                    };
            };
        };
    };

    return true;
}

const auto _ = register_server_tests();
