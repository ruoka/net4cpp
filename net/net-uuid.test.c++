module net;
import tester;
import std;

namespace {
using tester::assertions::check_eq;
using tester::assertions::check_true;
}

auto register_uuid_tests()
{
    tester::bdd::scenario("UUIDv4 format validation, [net]") = [] {
        tester::bdd::given("A generated UUIDv4") = [] {
            auto uuid = net::generate_uuid_v4();

            // Check length: should be exactly 36 characters (32 hex + 4 dashes)
            check_eq(uuid.size(), 36u);

            // Check format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
            check_eq(uuid[8], '-');
            check_eq(uuid[13], '-');
            check_eq(uuid[18], '-');
            check_eq(uuid[23], '-');

            // Check version 4 indicator: position 14 (index 13) should be '4'
            check_eq(uuid[14], '4');

            // Check variant indicator: position 19 (index 18) should be 8, 9, a, or b
            auto variant_char = uuid[19];
            check_true(variant_char == '8' || variant_char == '9' || 
                      variant_char == 'a' || variant_char == 'b' ||
                      variant_char == 'A' || variant_char == 'B');

            // Check all characters are valid hex or dashes
            for (auto i = 0uz; i < uuid.size(); ++i) {
                auto c = uuid[i];
                if (i == 8 || i == 13 || i == 18 || i == 23) {
                    check_eq(c, '-');
                } else {
                    check_true((c >= '0' && c <= '9') || 
                              (c >= 'a' && c <= 'f') || 
                              (c >= 'A' && c <= 'F'));
                }
            }
        };
    };

    tester::bdd::scenario("UUIDv4 uniqueness, [net]") = [] {
        tester::bdd::given("Multiple generated UUIDs") = [] {
            auto uuid1 = net::generate_uuid_v4();
            auto uuid2 = net::generate_uuid_v4();
            auto uuid3 = net::generate_uuid_v4();

            // All should be different
            check_true(uuid1 != uuid2);
            check_true(uuid2 != uuid3);
            check_true(uuid1 != uuid3);
        };
    };

    tester::bdd::scenario("UUIDv4 consistency, [net]") = [] {
        tester::bdd::given("Multiple generated UUIDs") = [] {
            std::set<std::string> uuids;
            for (int i = 0; i < 100; ++i) {
                auto uuid = net::generate_uuid_v4();
                
                // Each UUID should be unique in the set
                check_true(uuids.find(uuid) == uuids.end());
                uuids.insert(uuid);
                
                // Each should have correct format
                check_eq(uuid.size(), 36u);
                check_eq(uuid[14], '4');
            }
        };
    };

    return true;
}

const auto _ = register_uuid_tests();

