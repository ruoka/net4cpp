module net;
import tester;
import std;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
}

auto base64_test_reg = test_case("Base64") = [] {
    tester::bdd::scenario("To Character Set") = [] {
        check_eq('A', http::base64::to_character_set(0));
        check_eq('B', http::base64::to_character_set(1));
        check_eq('a', http::base64::to_character_set(26));
        check_eq('b', http::base64::to_character_set(27));
        check_eq('0', http::base64::to_character_set(52));
        check_eq('+', http::base64::to_character_set(62));
        check_eq('/', http::base64::to_character_set(63));
        check_eq('=', http::base64::to_character_set(64));
    };

    tester::bdd::scenario("Encode") = [] {
        check_eq("", http::base64::encode(""));
        check_eq("TQ==", http::base64::encode("M"));
        check_eq("TWE=", http::base64::encode("Ma"));
        check_eq("TWFu", http::base64::encode("Man"));

        check_eq("cGxlYXN1cmUu", http::base64::encode("pleasure."));
        check_eq("bGVhc3VyZS4=", http::base64::encode("leasure."));
        check_eq("ZWFzdXJlLg==", http::base64::encode("easure."));
        check_eq("YXN1cmUu", http::base64::encode("asure."));
        check_eq("c3VyZS4=", http::base64::encode("sure."));
    };

    tester::bdd::scenario("To Index") = [] {
        check_eq(0, http::base64::to_index('A'));
        check_eq(1, http::base64::to_index('B'));
        check_eq(26, http::base64::to_index('a'));
        check_eq(27, http::base64::to_index('b'));
        check_eq(52, http::base64::to_index('0'));
        check_eq(62, http::base64::to_index('+'));
        check_eq(63, http::base64::to_index('/'));
        check_eq(64, http::base64::to_index('='));
    };

    tester::bdd::scenario("Decode") = [] {
        check_eq("", http::base64::decode(""));
        check_eq("M", http::base64::decode("TQ=="));
        check_eq("Ma", http::base64::decode("TWE="));
        check_eq("Man", http::base64::decode("TWFu"));

        check_eq("pleasure.", http::base64::decode("cGxlYXN1cmUu"));
        check_eq("leasure.", http::base64::decode("bGVhc3VyZS4="));
        check_eq("easure.", http::base64::decode("ZWFzdXJlLg=="));
        check_eq("asure.", http::base64::decode("YXN1cmUu"));
        check_eq("sure.", http::base64::decode("c3VyZS4="));
    };
};

