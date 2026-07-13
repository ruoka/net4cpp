module net;
import :http_server_middlewares;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::check_contains;
using tester::assertions::check_nothrow;
} // namespace

auto register_middleware_tests()
{
    tester::bdd::scenario("body_size_validation_middleware - rejects oversized bodies, [net]") = [] {
        tester::bdd::given("A middleware with 100 byte limit") = [] {
            constexpr std::size_t max_size = 100;
            auto mw = ::http::middleware::body_size_validation_middleware(max_size);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request body exceeds limit") = [wrapped]() mutable {
                auto small_body_str = std::string(50, 'x');
                auto large_body_str = std::string(150, 'x');
                auto headers = ::http::headers{};
                
                auto req_view = "/test"sv;
                auto small_body_view = std::string_view{small_body_str};
                auto large_body_view = std::string_view{large_body_str};
                auto [status1, content1, headers1] = wrapped(req_view, small_body_view, headers);
                auto [status2, content2, headers2] = wrapped(req_view, large_body_view, headers);
                
                tester::bdd::then("Small body passes, large body is rejected") = [status1, content1, status2, content2] {
                    check_eq(status1, ::http::status_ok);
                    check_eq(content1, "OK"s);
                    check_eq(status2, ::http::status_payload_too_large);
                    check_contains(content2, "exceeds maximum size");
                    check_contains(content2, "100");
                };
            };
        };
    };

    tester::bdd::scenario("body_size_validation_middleware - allows bodies at limit, [net]") = [] {
        tester::bdd::given("A middleware with 100 byte limit") = [] {
            constexpr std::size_t max_size = 100;
            auto mw = ::http::middleware::body_size_validation_middleware(max_size);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request body is exactly at limit") = [wrapped]() mutable {
                auto exact_body_str = std::string(100, 'x');
                auto exact_body_view = std::string_view{exact_body_str};
                auto headers = ::http::headers{};
                
                auto req_view = "/test"sv;
                auto [status, content, headers_opt] = wrapped(req_view, exact_body_view, headers);
                
                tester::bdd::then("Request passes") = [status, content] {
                    check_eq(status, ::http::status_ok);
                    check_eq(content, "OK"s);
                };
            };
        };
    };

    tester::bdd::scenario("accept_validation_middleware - rejects incompatible Accept headers, [net]") = [] {
        tester::bdd::given("A middleware requiring application/json") = [] {
            auto mw = ::http::middleware::accept_validation_middleware("application/json"sv);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request has incompatible Accept header") = [wrapped]() mutable {
                auto headers1 = ::http::headers{};
                headers1.set("accept"s, "text/plain"s);
                
                auto headers2 = ::http::headers{};
                headers2.set("accept"s, "application/xml"s);
                
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto [status1, content1, headers_opt1] = wrapped(req_view, empty_body, headers1);
                auto [status2, content2, headers_opt2] = wrapped(req_view, empty_body, headers2);
                
                tester::bdd::then("Requests are rejected with 406") = [status1, content1, status2, content2] {
                    check_eq(status1, ::http::status_not_acceptable);
                    check_contains(content1, "Not Acceptable");
                    check_contains(content1, "application/json");
                    
                    check_eq(status2, ::http::status_not_acceptable);
                    check_contains(content2, "Not Acceptable");
                };
            };
        };
    };

    tester::bdd::scenario("accept_validation_middleware - accepts compatible Accept headers, [net]") = [] {
        tester::bdd::given("A middleware requiring application/json") = [] {
            auto mw = ::http::middleware::accept_validation_middleware("application/json"sv);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request has compatible Accept header") = [wrapped]() mutable {
                auto headers1 = ::http::headers{};
                // No Accept header - should accept anything
                
                auto headers2 = ::http::headers{};
                headers2.set("accept"s, "application/json"s);
                
                auto headers3 = ::http::headers{};
                headers3.set("accept"s, "*/*"s);
                
                auto headers4 = ::http::headers{};
                headers4.set("accept"s, "application/json, text/plain"s);
                
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto [status1, content1, headers_opt1] = wrapped(req_view, empty_body, headers1);
                auto [status2, content2, headers_opt2] = wrapped(req_view, empty_body, headers2);
                auto [status3, content3, headers_opt3] = wrapped(req_view, empty_body, headers3);
                auto [status4, content4, headers_opt4] = wrapped(req_view, empty_body, headers4);
                
                tester::bdd::then("All requests pass") = [status1, content1, status2, content2, status3, content3, status4, content4] {
                    check_eq(status1, ::http::status_ok);
                    check_eq(content1, "OK"s);
                    
                    check_eq(status2, ::http::status_ok);
                    check_eq(content2, "OK"s);
                    
                    check_eq(status3, ::http::status_ok);
                    check_eq(content3, "OK"s);
                    
                    check_eq(status4, ::http::status_ok);
                    check_eq(content4, "OK"s);
                };
            };
        };
    };

    tester::bdd::scenario("accept_validation_middleware - supports custom content types, [net]") = [] {
        tester::bdd::given("A middleware requiring text/xml") = [] {
            auto mw = ::http::middleware::accept_validation_middleware("text/xml"sv);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request has matching or wildcard Accept header") = [wrapped]() mutable {
                auto headers1 = ::http::headers{};
                headers1.set("accept"s, "text/xml"s);
                
                auto headers2 = ::http::headers{};
                headers2.set("accept"s, "*/*"s);
                
                auto headers3 = ::http::headers{};
                headers3.set("accept"s, "application/json"s);
                
                auto req_view = ::http::request_view{"/test"};
                auto empty_body = ::http::body_view{""sv};
                auto [status1, content1, headers_opt1] = wrapped(req_view, empty_body, headers1);
                auto [status2, content2, headers_opt2] = wrapped(req_view, empty_body, headers2);
                auto [status3, content3, headers_opt3] = wrapped(req_view, empty_body, headers3);
                
                tester::bdd::then("Matching and wildcard pass, others are rejected") = [status1, content1, status2, content2, status3, content3] {
                    check_eq(status1, ::http::status_ok);
                    check_eq(content1, "OK"s);
                    
                    check_eq(status2, ::http::status_ok);
                    check_eq(content2, "OK"s);
                    
                    check_eq(status3, ::http::status_not_acceptable);
                    check_contains(content3, "text/xml");
                };
            };
        };
    };

    tester::bdd::scenario("authentication_middleware - rejects unauthenticated requests, [net]") = [] {
        tester::bdd::given("A middleware requiring authentication") = [] {
            auto is_public = [](std::string_view path) -> bool {
                return path == "/public"sv;
            };
            
            auto valid_tokens = std::make_shared<std::set<std::string>>(std::set<std::string>{"Bearer token123", "Bearer admin"});
            auto validate_token = [valid_tokens](std::string_view token) -> bool {
                return valid_tokens->contains(std::string{token});
            };
            
            auto mw = ::http::middleware::authentication_middleware(is_public, validate_token, "Test Realm"sv);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request to protected path without token") = [wrapped]() mutable {
                auto req_view = "/protected"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Request is rejected with 401") = [status, content, headers_opt] {
                    check_eq(status, ::http::status_unauthorized);
                    check_eq(content, "Unauthorized"s);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_true(headers_opt.value().contains("www-authenticate"s));
                        check_contains(headers_opt.value()["www-authenticate"s], "Test Realm");
                    }
                };
            };
        };
    };

    tester::bdd::scenario("authentication_middleware - allows public paths, [net]") = [] {
        tester::bdd::given("A middleware with public paths") = [] {
            auto is_public = [](std::string_view path) -> bool {
                return path == "/public"sv || path == "/health"sv;
            };
            
            auto validate_token = [](std::string_view) -> bool { return false; };
            
            auto mw = ::http::middleware::authentication_middleware(is_public, validate_token);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request to public path") = [wrapped]() mutable {
                auto req_view = "/public"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Request passes without authentication") = [status, content] {
                    check_eq(status, ::http::status_ok);
                    check_eq(content, "OK"s);
                };
            };
        };
    };

    tester::bdd::scenario("authentication_middleware - validates tokens, [net]") = [] {
        tester::bdd::given("A middleware with token validation") = [] {
            auto is_public = [](std::string_view) -> bool { return false; };
            
            auto valid_tokens = std::make_shared<std::set<std::string>>(std::set<std::string>{"Bearer valid-token", "Bearer admin-key"});
            auto validate_token = [valid_tokens](std::string_view token) -> bool {
                return valid_tokens->contains(std::string{token});
            };
            
            auto mw = ::http::middleware::authentication_middleware(is_public, validate_token);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with valid token") = [wrapped]() mutable {
                auto req_view = "/protected"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("authorization"s, "Bearer valid-token"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Request passes") = [status, content] {
                    check_eq(status, ::http::status_ok);
                    check_eq(content, "OK"s);
                };
            };
        };
    };

    tester::bdd::scenario("authentication_middleware - rejects invalid tokens, [net]") = [] {
        tester::bdd::given("A middleware with token validation") = [] {
            auto is_public = [](std::string_view) -> bool { return false; };
            
            auto valid_tokens = std::make_shared<std::set<std::string>>(std::set<std::string>{"Bearer valid-token"});
            auto validate_token = [valid_tokens](std::string_view token) -> bool {
                return valid_tokens->contains(std::string{token});
            };
            
            auto mw = ::http::middleware::authentication_middleware(is_public, validate_token);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with invalid token") = [wrapped]() mutable {
                auto req_view = "/protected"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("authorization"s, "Bearer invalid-token"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Request is rejected with 401") = [status, content] {
                    check_eq(status, ::http::status_unauthorized);
                    check_eq(content, "Unauthorized"s);
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - adds CORS headers to responses, [net]") = [] {
        tester::bdd::given("A CORS middleware with default config") = [] {
            auto allowed_origin = [](std::string_view) { return true; };
            auto mw = ::http::middleware::cors_middleware(allowed_origin);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with Origin header") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("origin"s, "https://example.com"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response includes CORS headers") = [status, content, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_eq(content, "OK"s);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        const auto& h = headers_opt.value();
                        check_true(h.contains("access-control-allow-origin"s));
                        check_eq(h["access-control-allow-origin"s], "https://example.com"s);
                        check_true(h.contains("access-control-allow-methods"s));
                        check_true(h.contains("access-control-allow-headers"s));
                        check_true(h.contains("access-control-allow-credentials"s));
                        check_eq(h["access-control-allow-credentials"s], "true"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - uses wildcard when no Origin header, [net]") = [] {
        tester::bdd::given("A CORS middleware with default config") = [] {
            auto allowed_origin = [](std::string_view) { return true; };
            auto mw = ::http::middleware::cors_middleware(allowed_origin);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request without Origin header") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response uses wildcard origin") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_eq(headers_opt.value()["access-control-allow-origin"s], "*"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - respects allowed origin policy, [net]") = [] {
        tester::bdd::given("A CORS middleware with restricted origins") = [] {
            auto allowed_origins = std::make_shared<std::set<std::string>>(std::set<std::string>{"https://example.com", "https://trusted.com"});
            auto allowed_origin = [allowed_origins](std::string_view origin) -> bool {
                return allowed_origins->contains(std::string{origin});
            };
            
            auto mw = ::http::middleware::cors_middleware(allowed_origin);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with allowed origin") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("origin"s, "https://example.com"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response includes allowed origin") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_eq(headers_opt.value()["access-control-allow-origin"s], "https://example.com"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - uses wildcard for disallowed origin, [net]") = [] {
        tester::bdd::given("A CORS middleware with restricted origins") = [] {
            auto allowed_origins = std::make_shared<std::set<std::string>>(std::set<std::string>{"https://example.com"});
            auto allowed_origin = [allowed_origins](std::string_view origin) -> bool {
                return allowed_origins->contains(std::string{origin});
            };
            
            auto mw = ::http::middleware::cors_middleware(allowed_origin);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with disallowed origin") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("origin"s, "https://evil.com"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response uses wildcard origin") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_eq(headers_opt.value()["access-control-allow-origin"s], "*"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - includes max-age when configured, [net]") = [] {
        tester::bdd::given("A CORS middleware with max-age") = [] {
            auto allowed_origin = [](std::string_view) { return true; };
            auto mw = ::http::middleware::cors_middleware(allowed_origin, {}, {}, true, std::make_optional(3600));
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request is made") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response includes max-age header") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_true(headers_opt.value().contains("access-control-max-age"s));
                        check_eq(headers_opt.value()["access-control-max-age"s], "3600"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("cors_middleware - preserves existing response headers, [net]") = [] {
        tester::bdd::given("A CORS middleware") = [] {
            auto allowed_origin = [](std::string_view) { return true; };
            auto mw = ::http::middleware::cors_middleware(allowed_origin);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                auto existing_headers = ::http::headers{};
                existing_headers.set("custom-header"s, "custom-value"s);
                return ::http::make_response(::http::status_ok, "OK"s, std::make_optional(existing_headers));
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request is made") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response includes both CORS and custom headers") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        const auto& h = headers_opt.value();
                        check_true(h.contains("custom-header"s));
                        check_eq(h["custom-header"s], "custom-value"s);
                        check_true(h.contains("access-control-allow-origin"s));
                    }
                };
            };
        };
    };

    tester::bdd::scenario("correlation_id_middleware - generates correlation ID when missing, [net]") = [] {
        tester::bdd::given("A correlation ID middleware") = [] {
            auto mw = ::http::middleware::correlation_id_middleware();
            
            auto handler = [](auto&&, auto&&, const auto& hdr) -> ::http::response_with_headers {
                using namespace std::string_literals;
                auto headers = ::http::headers{};
                if(hdr.contains("x-correlation-id"s))
                {
                    headers.set("x-correlation-id"s, hdr["x-correlation-id"s]);
                }
                return ::http::make_response(::http::status_ok, "OK"s, std::make_optional(headers));
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request without X-Correlation-ID header") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response includes generated correlation ID") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        const auto& h = headers_opt.value();
                        check_true(h.contains("x-correlation-id"s));
                        const auto correlation_id = h["x-correlation-id"s];
                        // UUIDv4 format: 8-4-4-4-12 hex digits
                        check_true(correlation_id.size() == 36); // UUID format length
                        check_true(correlation_id.find('-') != std::string::npos);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("correlation_id_middleware - preserves existing correlation ID, [net]") = [] {
        tester::bdd::given("A correlation ID middleware") = [] {
            auto mw = ::http::middleware::correlation_id_middleware();
            
            auto handler = [](auto&&, auto&&, const auto& hdr) -> ::http::response_with_headers {
                using namespace std::string_literals;
                auto headers = ::http::headers{};
                if(hdr.contains("x-correlation-id"s))
                {
                    headers.set("x-correlation-id"s, hdr["x-correlation-id"s]);
                }
                return ::http::make_response(::http::status_ok, "OK"s, std::make_optional(headers));
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with existing X-Correlation-ID header") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("x-correlation-id"s, "existing-id-12345"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response preserves existing correlation ID") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_eq(headers_opt.value()["x-correlation-id"s], "existing-id-12345"s);
                    }
                };
            };
        };
    };

    tester::bdd::scenario("correlation_id_middleware - preserves all other headers, [net]") = [] {
        tester::bdd::given("A correlation ID middleware") = [] {
            auto mw = ::http::middleware::correlation_id_middleware();
            
            auto handler = [](auto&&, auto&&, const auto& hdr) -> ::http::response_with_headers {
                using namespace std::string_literals;
                auto response_headers = ::http::headers{};
                // Copy all headers to response
                for(const auto& [name, value] : hdr)
                {
                    response_headers.set(name, value);
                }
                return ::http::make_response(::http::status_ok, "OK"s, std::make_optional(response_headers));
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Request with other headers") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                headers.set("authorization"s, "Bearer token123"s);
                headers.set("content-type"s, "application/json"s);
                
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Response preserves all headers and adds correlation ID") = [status, headers_opt] {
                    check_eq(status, ::http::status_ok);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        const auto& h = headers_opt.value();
                        check_true(h.contains("authorization"s));
                        check_eq(h["authorization"s], "Bearer token123"s);
                        check_true(h.contains("content-type"s));
                        check_eq(h["content-type"s], "application/json"s);
                        check_true(h.contains("x-correlation-id"s));
                    }
                };
            };
        };
    };

    tester::bdd::scenario("rate_limiting_middleware - allows requests within limit, [net]") = [] {
        tester::bdd::given("A rate limiting middleware with limit of 5 requests per 60 seconds") = [] {
            auto key_extractor = [](std::string_view, const ::http::headers&) { return "test-key"s; };
            
            auto mw = ::http::middleware::rate_limiting_middleware(5, std::chrono::seconds{60}, key_extractor);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Making requests within limit") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                // Make 5 requests (within limit)
                for(int i = 0; i < 5; ++i)
                {
                    auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                    tester::bdd::then("All requests succeed") = [status] {
                        check_eq(status, ::http::status_ok);
                    };
                }
            };
        };
    };

    tester::bdd::scenario("rate_limiting_middleware - rejects requests exceeding limit, [net]") = [] {
        tester::bdd::given("A rate limiting middleware with limit of 2 requests per 60 seconds") = [] {
            auto key_extractor = [](std::string_view, const ::http::headers&) { return "test-key-2"s; };
            
            auto mw = ::http::middleware::rate_limiting_middleware(2, std::chrono::seconds{60}, key_extractor);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Making requests exceeding limit") = [wrapped]() mutable {
                auto req_view = "/test"sv;
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                // Make 2 requests (within limit)
                for(int i = 0; i < 2; ++i)
                {
                    auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                    check_eq(status, ::http::status_ok);
                }
                
                // Third request should be rate limited
                auto [status, content, headers_opt] = wrapped(req_view, empty_body, headers);
                
                tester::bdd::then("Request is rejected with 429") = [status, content, headers_opt] {
                    check_eq(status, ::http::status_too_many_requests);
                    check_true(content.find("Too Many Requests") != std::string::npos);
                    check_true(headers_opt.has_value());
                    if(headers_opt.has_value())
                    {
                        check_true(headers_opt.value().contains("retry-after"s));
                    }
                };
            };
        };
    };

    tester::bdd::scenario("rate_limiting_middleware - uses key extractor for separate limits, [net]") = [] {
        tester::bdd::given("A rate limiting middleware with key extractor") = [] {
            auto key_extractor = [](std::string_view req, const ::http::headers&) {
                // Extract key from request path
                return std::string{req};
            };
            
            auto mw = ::http::middleware::rate_limiting_middleware(1, std::chrono::seconds{60}, key_extractor);
            
            auto handler = [](auto&&, auto&&, auto&&) -> ::http::response_with_headers {
                return ::http::make_response(::http::status_ok, "OK"s, std::optional<::http::headers>{});
            };
            
            auto wrapped = mw(handler);
            
            tester::bdd::when("Making requests with different keys") = [wrapped]() mutable {
                auto empty_body = ""sv;
                auto headers = ::http::headers{};
                
                // First request to /path1
                auto [status1, content1, headers_opt1] = wrapped("/path1"sv, empty_body, headers);
                check_eq(status1, ::http::status_ok);
                
                // Second request to /path2 (different key, should succeed)
                auto [status2, content2, headers_opt2] = wrapped("/path2"sv, empty_body, headers);
                
                tester::bdd::then("Different keys have separate limits") = [status2] {
                    check_eq(status2, ::http::status_ok);
                };
            };
        };
    };

    return true;
}

const auto _ = register_middleware_tests();

