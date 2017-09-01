#include <gtest/gtest.h>
#include <string_view>
#include "cryptic/sha1.hpp"

using namespace std::string_literals;

TEST(CrypticSHA1,base64)
{
    auto test1 = ""s;
    EXPECT_EQ("2jmj7l5rSw0yVb/vlWAYkK/YBwk="s, cryptic::sha1::base64(test1));
    EXPECT_EQ("da39a3ee5e6b4b0d3255bfef95601890afd80709"s, cryptic::sha1::hexadecimal(test1));

    auto test2 = "The quick brown fox jumps over the lazy dog"s;
    EXPECT_EQ("L9ThxnotKPzthJ7hu3bnORuT6xI="s, cryptic::sha1::base64(test2));
    EXPECT_EQ("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"s, cryptic::sha1::hexadecimal(test2));

    auto test3 = "The quick brown fox jumps over the lazy cog"s;
    EXPECT_EQ("3p8sf9JeGzr60+haC9F9mxANtLM="s, cryptic::sha1::base64(test3));
    EXPECT_EQ("de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3"s, cryptic::sha1::hexadecimal(test3));
}
