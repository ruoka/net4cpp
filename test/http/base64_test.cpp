#include <gtest/gtest.h>
#include "http/base64.hpp"

TEST(HttpBase64,ToCharacterSet)
{
    ASSERT_EQ('A', http::base64::to_character_set(0));
    ASSERT_EQ('B', http::base64::to_character_set(1));
    ASSERT_EQ('a', http::base64::to_character_set(26));
    ASSERT_EQ('b', http::base64::to_character_set(27));
    ASSERT_EQ('0', http::base64::to_character_set(52));
    ASSERT_EQ('+', http::base64::to_character_set(62));
    ASSERT_EQ('/', http::base64::to_character_set(63));
    ASSERT_EQ('=', http::base64::to_character_set(64));
}

TEST(HttpBase64,Encode)
{
    EXPECT_EQ("", http::base64::encode(""));
    EXPECT_EQ("TQ==", http::base64::encode("M"));
    EXPECT_EQ("TWE=", http::base64::encode("Ma"));
    EXPECT_EQ("TWFu", http::base64::encode("Man"));

    EXPECT_EQ("cGxlYXN1cmUu", http::base64::encode("pleasure."));
    EXPECT_EQ("bGVhc3VyZS4=", http::base64::encode("leasure."));
    EXPECT_EQ("ZWFzdXJlLg==", http::base64::encode("easure."));
    EXPECT_EQ("YXN1cmUu", http::base64::encode("asure."));
    EXPECT_EQ("c3VyZS4=", http::base64::encode("sure."));
}

TEST(HttpBase64,ToIndex)
{
    ASSERT_EQ(0, http::base64::to_index('A'));
    ASSERT_EQ(1, http::base64::to_index('B'));
    ASSERT_EQ(26, http::base64::to_index('a'));
    ASSERT_EQ(27, http::base64::to_index('b'));
    ASSERT_EQ(52, http::base64::to_index('0'));
    ASSERT_EQ(62, http::base64::to_index('+'));
    ASSERT_EQ(63, http::base64::to_index('/'));
    ASSERT_EQ(64, http::base64::to_index('='));
}

TEST(HttpBase64,Decode)
{
    EXPECT_EQ("", http::base64::decode(""));
    EXPECT_EQ("M", http::base64::decode("TQ=="));
    EXPECT_EQ("Ma", http::base64::decode("TWE="));
    EXPECT_EQ("Man", http::base64::decode("TWFu"));

    EXPECT_EQ("pleasure.", http::base64::decode("cGxlYXN1cmUu"));
    EXPECT_EQ("leasure.", http::base64::decode("bGVhc3VyZS4="));
    EXPECT_EQ("easure.", http::base64::decode("ZWFzdXJlLg=="));
    EXPECT_EQ("asure.", http::base64::decode("YXN1cmUu"));
    EXPECT_EQ("sure.", http::base64::decode("c3VyZS4="));
}
