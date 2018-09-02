// Copyright (c) 2017 Kaius Ruokonen
// Distributed under the MIT software license
// See LICENCE file or https://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <string_view>
#include "cryptic/base64.hpp"

using namespace std::string_literals;

TEST(CrypticBase64,ToCharacterSet)
{
    ASSERT_EQ('A', cryptic::base64::to_character_set(std::byte{0}));
    ASSERT_EQ('B', cryptic::base64::to_character_set(std::byte{1}));
    ASSERT_EQ('a', cryptic::base64::to_character_set(std::byte{26}));
    ASSERT_EQ('b', cryptic::base64::to_character_set(std::byte{27}));
    ASSERT_EQ('0', cryptic::base64::to_character_set(std::byte{52}));
    ASSERT_EQ('+', cryptic::base64::to_character_set(std::byte{62}));
    ASSERT_EQ('/', cryptic::base64::to_character_set(std::byte{63}));
    ASSERT_EQ('=', cryptic::base64::to_character_set(std::byte{64}));
}

TEST(CrypticBase64,Encode)
{
    EXPECT_EQ(""s, cryptic::base64::encode(""s));
    EXPECT_EQ("TQ=="s, cryptic::base64::encode("M"s));
    EXPECT_EQ("TWE="s, cryptic::base64::encode("Ma"s));
    EXPECT_EQ("TWFu"s, cryptic::base64::encode("Man"s));

    EXPECT_EQ("cGxlYXN1cmUu"s, cryptic::base64::encode("pleasure."s));
    EXPECT_EQ("bGVhc3VyZS4="s, cryptic::base64::encode("leasure."s));
    EXPECT_EQ("ZWFzdXJlLg=="s, cryptic::base64::encode("easure."s));
    EXPECT_EQ("YXN1cmUu"s, cryptic::base64::encode("asure."s));
    EXPECT_EQ("c3VyZS4="s, cryptic::base64::encode("sure."s));
}

TEST(CrypticBase64,ToIndex)
{
    ASSERT_EQ(0, cryptic::base64::to_index('A'));
    ASSERT_EQ(1, cryptic::base64::to_index('B'));
    ASSERT_EQ(26, cryptic::base64::to_index('a'));
    ASSERT_EQ(27, cryptic::base64::to_index('b'));
    ASSERT_EQ(52, cryptic::base64::to_index('0'));
    ASSERT_EQ(62, cryptic::base64::to_index('+'));
    ASSERT_EQ(63, cryptic::base64::to_index('/'));
    ASSERT_EQ(64, cryptic::base64::to_index('='));
}

TEST(CrypticBase64,Decode)
{
    EXPECT_EQ("", cryptic::base64::decode(""));
    EXPECT_EQ("M", cryptic::base64::decode("TQ=="));
    EXPECT_EQ("Ma", cryptic::base64::decode("TWE="));
    EXPECT_EQ("Man", cryptic::base64::decode("TWFu"));

    EXPECT_EQ("pleasure.", cryptic::base64::decode("cGxlYXN1cmUu"));
    EXPECT_EQ("leasure.", cryptic::base64::decode("bGVhc3VyZS4="));
    EXPECT_EQ("easure.", cryptic::base64::decode("ZWFzdXJlLg=="));
    EXPECT_EQ("asure.", cryptic::base64::decode("YXN1cmUu"));
    EXPECT_EQ("sure.", cryptic::base64::decode("c3VyZS4="));
}
