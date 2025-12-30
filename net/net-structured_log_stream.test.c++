module net;
import tester;
import std;

using namespace net;

namespace {
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;
using tester::assertions::check_contains;
using tester::assertions::check_starts_with;

[[maybe_unused]] inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_syslogstream_tests()
{
    tester::bdd::scenario("Timestamp formatting, [net]") = [] {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        check_eq("1970-01-01T00:00:00.000Z", syslog::format_timestamp(system_clock::time_point{0us}));
        check_eq("1970-01-01T00:00:00.001Z", syslog::format_timestamp(system_clock::time_point{1ms}));
        check_eq("1970-01-01T00:00:01.000Z", syslog::format_timestamp(system_clock::time_point{1s}));
        check_eq("1970-01-01T00:01:00.000Z", syslog::format_timestamp(system_clock::time_point{1min}));
        check_eq("1970-01-01T01:00:00.000Z", syslog::format_timestamp(system_clock::time_point{1h}));
    };

    tester::bdd::scenario("Setup and Log levels, [net]") = [] {
        tester::bdd::given("A syslog stream setup") = [] {
            slog.appname("tester");
            slog.facility(syslog::facility::local0);
            slog.level(syslog::severity::debug);
            slog.redirect(std::clog);

            tester::bdd::when("Logging at various levels") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << net::debug() << "Testing debug "s << 123 << net::flush;
                    slog << net::debug("http") << "Testing debug http "s << 123 << net::flush;
                    
                    slog << net::info() << "Testing info "s << 456 << net::flush;
                    slog << net::info("http") << "Testing info http "s << 456 << net::flush;
                    
                    slog << net::notice() << "Testing notice "s << "+ 123 +" << net::flush;
                    slog << net::notice("http") << "Testing notice http "s << "+ 123 +" << net::flush;
                    
                    slog << net::warning() << "Testing warning "s << 789.0 << net::flush;
                    slog << net::warning("http") << "Testing warning http "s << 789.0 << net::flush;
                    
                    slog << net::error << "Testing error "s << true << 321 << false << net::flush;
                    slog << net::error("http") << "Testing error http "s << true << 321 << false << net::flush;
                });
            };
        };
    };

    tester::bdd::scenario("Backward compatibility with syslog::helper, [net]") = [] {
        tester::bdd::given("A syslog stream setup") = [] {
            slog.appname("tester");
            slog.level(syslog::severity::debug);
            slog.redirect(std::clog);

            tester::bdd::when("Using syslog::helper API") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << syslog::helper{syslog::severity::debug, "test"} << "Debug message "s << 123 << net::flush;
                    slog << syslog::helper{syslog::severity::info, "test"} << "Info message "s << 456 << net::flush;
                    slog << syslog::severity::warning << "Warning message "s << 789 << net::flush;
                });
            };
        };
    };

    tester::bdd::scenario("Structured logging with fields, [net]") = [] {
        tester::bdd::given("A syslog stream setup") = [] {
            slog.appname("tester");
            slog.level(syslog::severity::debug);
            slog.redirect(std::clog);

            tester::bdd::when("Adding structured fields") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << net::info("http") 
                         << std::pair{"method", "GET"}
                         << std::pair{"path", "/api/users"}
                         << std::pair{"status", 200}
                         << std::pair{"duration_ms", 45.5}
                         << "Request completed" << net::flush;
                });
            };

            tester::bdd::when("Adding structured fields using new style syntax") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    // New style: std::pair{"name", value} with CTAD
                    slog << net::info("SERVER_START") 
                         << "Listening on port 21120" 
                         << std::pair{"addr", "0.0.0.0"} 
                         << net::flush;
                });
            };

            tester::bdd::when("Adding structured fields with error message") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    std::string client_ip = "192.168.1.100";
                    slog << net::error("CONN_CLOSED") 
                         << "Timeout from client" 
                         << std::pair{"ip", client_ip} 
                         << net::flush;
                });
            };

            tester::bdd::when("Adding structured fields using field() helper") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << net::info("http") 
                         << net::field("method", "GET")
                         << net::field("path", "/api/users")
                         << net::field("status", 200)
                         << net::field("duration_ms", 45.5)
                         << "Request completed" << net::flush;
                });
            };
        };
    };

    tester::bdd::scenario("Structured fields output verification, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for JSONL format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.sd_id("api");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with structured fields using new style syntax") = [captured_output] {
                using namespace std::string_literals;
                slog << net::info("SERVER_START") 
                     << "Listening on port 21120" 
                     << std::pair{"addr", "0.0.0.0"} 
                     << std::pair{"port", 21120}
                     << net::flush;

                tester::bdd::then("Output should contain structured fields") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"addr\":\"0.0.0.0\"");
                    check_contains(output, "\"port\":21120");
                    check_contains(output, "\"msg_id\":\"SERVER_START\"");
                    check_contains(output, "Listening on port 21120");
                    // Reset redirect to default to avoid dangling pointer
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Structured fields with error message output verification, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for JSONL format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.sd_id("api");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging error with structured fields") = [captured_output] {
                using namespace std::string_literals;
                std::string client_ip = "192.168.1.100";
                slog << net::error("CONN_CLOSED") 
                     << "Timeout from client" 
                     << std::pair{"ip", client_ip}
                     << std::pair{"timeout_sec", 30}
                     << net::flush;

                tester::bdd::then("Output should contain error level and structured fields") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"level\":\"error\"");
                    check_contains(output, "\"msg_id\":\"CONN_CLOSED\"");
                    check_contains(output, "\"ip\":\"192.168.1.100\"");
                    check_contains(output, "\"timeout_sec\":30");
                    check_contains(output, "Timeout from client");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Structured fields in syslog format output verification, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for syslog format") = [captured_output] {
            slog.appname("testapp");
            slog.facility(syslog::facility::local0);
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::syslog);
            slog.sd_id("test@12345");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with structured fields using new style syntax") = [captured_output] {
                using namespace std::string_literals;
                slog << net::info("SERVER_START") 
                     << "Listening on port 21120" 
                     << std::pair{"addr", "0.0.0.0"} 
                     << std::pair{"port", 21120}
                     << net::flush;

                tester::bdd::then("Output should contain structured data in syslog format") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "[test@12345");
                    check_contains(output, "addr=");
                    check_contains(output, "port=");
                    check_contains(output, "0.0.0.0");
                    check_contains(output, "21120");
                    check_contains(output, "SERVER_START");
                    check_contains(output, "Listening on port 21120");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Log level filtering, [net]") = [] {
        tester::bdd::given("A syslog stream with info level") = [] {
            slog.appname("tester");
            slog.level(syslog::severity::info);
            slog.redirect(std::clog);

            tester::bdd::when("Logging at different levels") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    // These should be filtered out (below info level)
                    slog << net::debug << "This debug message should be filtered" << net::flush;
                    
                    // These should be logged (info and above)
                    slog << net::info << "This info message should be logged" << net::flush;
                    slog << net::warning << "This warning message should be logged" << net::flush;
                    slog << net::error << "This error message should be logged" << net::flush;
                });
            };
        };
    };

    tester::bdd::scenario("JSONL format output, [net]") = [] {
        tester::bdd::given("A syslog stream configured for JSONL format") = [] {
            slog.appname("tester");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.redirect(std::clog);

            tester::bdd::when("Logging a message") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << net::info("test_event")
                         << std::pair{"user_id", 12345}
                         << std::pair{"action", "login"}
                         << "User logged in" << net::flush;
                });
            };

            // Reset to syslog format
            slog.set_format(log_format::syslog);
        };
    };

    tester::bdd::scenario("Facility configuration, [net]") = [] {
        tester::bdd::given("A syslog stream") = [] {
            slog.appname("tester");
            slog.level(syslog::severity::debug);

            tester::bdd::when("Setting different facilities") = [] {
                check_nothrow([] {
                    slog.facility(syslog::facility::local0);
                    slog.facility(syslog::facility::local1);
                    slog.facility(static_cast<unsigned>(16));  // local0
                    slog.facility(static_cast<unsigned>(23));  // local7
                });
            };
        };
    };

    tester::bdd::scenario("App name method, [net]") = [] {
        tester::bdd::given("A syslog stream") = [] {
            tester::bdd::when("Setting app name") = [] {
                check_nothrow([] {
                    slog.appname("myapp");
                });
            };
        };
    };

    tester::bdd::scenario("Syslog format output verification, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for syslog format") = [captured_output] {
            slog.appname("testapp");
            slog.facility(syslog::facility::local0);
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::syslog);
            slog.redirect(*captured_output);

            tester::bdd::when("Logging a simple message") = [captured_output] {
                using namespace std::string_literals;
                slog << net::info("test_msg") << "Simple test message"s << net::flush;

                tester::bdd::then("Output should be in RFC 5424 syslog format") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "<");
                    check_contains(output, ">1 ");
                    check_contains(output, "testapp");
                    check_contains(output, "test_msg");
                    check_contains(output, "Simple test message");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Syslog format with structured data, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for syslog format") = [captured_output] {
            slog.appname("testapp");
            slog.facility(syslog::facility::local0);
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::syslog);
            slog.sd_id("test@12345");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with structured fields") = [captured_output] {
                using namespace std::string_literals;
                slog << net::warning("http_request")
                     << std::pair{"method", "POST"}
                     << std::pair{"path", "/api/users"}
                     << std::pair{"status", 404}
                     << "Request failed"s << net::flush;

                tester::bdd::then("Output should contain structured data") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "[test@12345");
                    check_contains(output, "method=");
                    check_contains(output, "path=");
                    check_contains(output, "status=");
                    check_contains(output, "/api/users");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("JSONL format output verification, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for JSONL format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.sd_id("test_source");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging a simple message") = [captured_output] {
                using namespace std::string_literals;
                slog << net::info("test_event") << "User action completed"s << net::flush;

                tester::bdd::then("Output should be valid JSONL") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"time\":");
                    check_contains(output, "\"host\":");
                    check_contains(output, "\"app\":\"testapp\"");
                    check_contains(output, "\"level\":\"info\"");
                    check_contains(output, "\"source\":\"test_source\"");
                    check_contains(output, "\"msg_id\":\"test_event\"");
                    check_contains(output, "\"message\":");
                    check_contains(output, "User action completed");
                    check_starts_with(output, "{");
                    check_contains(output, "}");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("JSONL format with structured fields, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for JSONL format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.sd_id("api");
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with structured fields") = [captured_output] {
                using namespace std::string_literals;
                slog << net::error("api_error")
                     << std::pair{"error_code", 500}
                     << std::pair{"endpoint", "/api/data"}
                     << std::pair{"retry_count", 3}
                     << std::pair{"timeout", true}
                     << "API request failed"s << net::flush;

                tester::bdd::then("Output should contain structured fields in JSON") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"error_code\":500");
                    check_contains(output, "\"endpoint\":\"/api/data\"");
                    check_contains(output, "\"retry_count\":3");
                    check_contains(output, "\"timeout\":true");
                    check_contains(output, "\"level\":\"error\"");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Format switching between syslog and JSONL, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.redirect(*captured_output);

            tester::bdd::when("Switching between formats") = [captured_output] {
                using namespace std::string_literals;
                
                // First log in syslog format
                slog.set_format(log_format::syslog);
                slog << net::info("format_test") << "Syslog format message"s << net::flush;
                
                // Then switch to JSONL
                slog.set_format(log_format::jsonl);
                slog << net::info("format_test") << "JSONL format message"s << net::flush;

                tester::bdd::then("Both formats should be present in output") = [captured_output] {
                    std::string output = captured_output->str();
                    // Check for syslog format markers
                    check_contains(output, ">1 ");
                    // Check for JSONL format markers
                    check_contains(output, "\"time\":");
                    check_contains(output, "Syslog format message");
                    check_contains(output, "JSONL format message");
                    slog.redirect(std::clog);
                };
            };

            // Reset to syslog format
            slog.set_format(log_format::syslog);
        };
    };

    tester::bdd::scenario("JSONL format with different value types, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for JSONL format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::jsonl);
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with various value types") = [captured_output] {
                using namespace std::string_literals;
                slog << net::info("type_test")
                     << std::pair{"string_val", "test_string"}
                     << std::pair{"int_val", 42}
                     << std::pair{"double_val", 3.14}
                     << std::pair{"bool_val", true}
                     << "Type test message"s << net::flush;

                tester::bdd::then("All value types should be correctly formatted") = [captured_output] {
                    std::string output = captured_output->str();
                    check_contains(output, "\"string_val\":\"test_string\"");
                    check_contains(output, "\"int_val\":42");
                    check_contains(output, "\"double_val\":3.14");
                    check_contains(output, "\"bool_val\":true");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    tester::bdd::scenario("Syslog format priority calculation, [net]") = [] {
        auto captured_output = std::make_shared<std::stringstream>();
        
        tester::bdd::given("A syslog stream configured for syslog format") = [captured_output] {
            slog.appname("testapp");
            slog.level(syslog::severity::debug);
            slog.set_format(log_format::syslog);
            slog.redirect(*captured_output);

            tester::bdd::when("Logging with different facilities and severities") = [captured_output] {
                using namespace std::string_literals;
                
                // Test with local0 (16) and info (6) = priority 16*8+6 = 134
                slog.facility(syslog::facility::local0);
                slog << net::info("test") << "Info message"s << net::flush;
                
                // Test with user (1) and error (3) = priority 1*8+3 = 11
                slog.facility(syslog::facility::user);
                slog << net::error("test") << "Error message"s << net::flush;

                tester::bdd::then("Priority values should be correct") = [captured_output] {
                    std::string output = captured_output->str();
                    // Priority for local0 + info = 16*8+6 = 134
                    check_contains(output, "<134>");
                    // Priority for user + error = 1*8+3 = 11
                    check_contains(output, "<11>");
                    slog.redirect(std::clog);
                };
            };
        };
    };

    return true;
}

const auto _ = register_syslogstream_tests();
