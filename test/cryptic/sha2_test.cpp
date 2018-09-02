#include <gtest/gtest.h>
#include <string_view>
#include "cryptic/sha2.hpp"

using namespace std::string_literals;

TEST(CrypticSHA224,base64)
{
    auto hash = cryptic::sha224{};
    EXPECT_EQ(28, hash.size());

    auto test1 = ""s;
    EXPECT_EQ("0UoCjCo6K8lHYQK7KII0xBWisB+CjqYqxbPkLw=="s, cryptic::sha224::base64(test1));
    EXPECT_EQ("d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"s, cryptic::sha224::hexadecimal(test1));

    auto test2 = "The quick brown fox jumps over the lazy dog"s;
    EXPECT_EQ("cw4Qm9eooyscudmgmqIyXSQwWH3bwMOLrZEVJQ=="s, cryptic::sha224::base64(test2));
    EXPECT_EQ("730e109bd7a8a32b1cb9d9a09aa2325d2430587ddbc0c38bad911525"s, cryptic::sha224::hexadecimal(test2));

    auto test3 = "The quick brown fox jumps over the lazy cog"s;
    EXPECT_EQ("/udV9EpV8g+zNizcPEk2FbPLV07ZXOYQ7lsemw=="s, cryptic::sha224::base64(test3));
    EXPECT_EQ("fee755f44a55f20fb3362cdc3c493615b3cb574ed95ce610ee5b1e9b"s, cryptic::sha224::hexadecimal(test3));
}

TEST(CrypticSHA256,base64)
{
    auto hash = cryptic::sha256{};
    EXPECT_EQ(32, hash.size());

    auto test1 = ""s;
    EXPECT_EQ("47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU="s, cryptic::sha256::base64(test1));
    EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"s, cryptic::sha256::hexadecimal(test1));

    auto test2 = "The quick brown fox jumps over the lazy dog"s;
    EXPECT_EQ("16j7swfXgJRpypq8sAguT41WUeRtPNt2LQLQvzfJ5ZI="s, cryptic::sha256::base64(test2));
    EXPECT_EQ("d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"s, cryptic::sha256::hexadecimal(test2));

    auto test3 = "The quick brown fox jumps over the lazy cog"s;
    EXPECT_EQ("5MTY8792tpLeeRoXPgUyEVD3o0W0ZIT+Qn9qzH7Mgb4="s, cryptic::sha256::base64(test3));
    EXPECT_EQ("e4c4d8f3bf76b692de791a173e05321150f7a345b46484fe427f6acc7ecc81be"s, cryptic::sha256::hexadecimal(test3));
}
