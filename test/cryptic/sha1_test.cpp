#include <gtest/gtest.h>
#include <string_view>
#include "cryptic/sha1.hpp"

using namespace std::string_literals;

TEST(CrypticSHA1,base64)
{
    auto test1 = ""s;
    EXPECT_EQ("2jmj7l5rSw0yVb/vlWAYkK/YBwk="s, cryptic::sha1::base64(test1));

    auto test2 = "The quick brown fox jumps over the lazy dog"s;
    EXPECT_EQ("L9ThxnotKPzthJ7hu3bnORuT6xI="s, cryptic::sha1::base64(test2));

    auto test3 = "The quick brown fox jumps over the lazy cog"s;
    EXPECT_EQ("3p8sf9JeGzr60+haC9F9mxANtLM="s, cryptic::sha1::base64(test3));
}
