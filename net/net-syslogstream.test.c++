module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto syslogstream_test_reg = test_case("Syslog Stream") = [] {
    if(!network_tests_enabled()) return;
    tester::bdd::scenario("Setup and Log levels") = [] {
        tester::bdd::given("A syslog stream setup") = [] {
            slog.appname("tester");
            slog.facility(syslog::facility::local0);
            slog.level(syslog::severity::debug);
            slog.redirect(std::clog);

            tester::bdd::when("Logging at various levels") = [] {
                using namespace std::string_literals;
                check_nothrow([] {
                    slog << net::debug << "Testing debug "s << 123 << net::flush;
                    slog << net::debug("http") << "Testing debug http "s << 123 << net::flush;
                    
                    slog << net::info << "Testing info "s << 456 << net::flush;
                    slog << net::info("http") << "Testing info http "s << 456 << net::flush;
                    
                    slog << net::notice << "Testing notice "s << "+ 123 +" << net::flush;
                    slog << net::notice("http") << "Testing notice http "s << "+ 123 +" << net::flush;
                    
                    slog << net::warning << "Testing warning "s << 789.0 << net::flush;
                    slog << net::warning("http") << "Testing warning http "s << 789.0 << net::flush;
                    
                    slog << net::error << "Testing error "s << true << 321 << false << net::flush;
                    slog << net::error("http") << "Testing error http "s << true << 321 << false << net::flush;
                });
            };
        };
    };
};

