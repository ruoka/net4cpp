// Copyright (c) 2017 Kaius Ruokonen
// Distributed under the MIT software license
// See LICENCE file or https://opensource.org/licenses/MIT

#pragma once
#include <string>
#include <array>
#include <span>
#include <algorithm>
#include "gsl/assert.hpp"

namespace cryptic::base64 {

    inline constexpr char to_character_set(std::byte b)
    {
        assert(b < std::byte{65});
        constexpr auto set = std::to_array("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
        return set[std::to_integer<std::size_t>(b)];
    }

    inline constexpr char to_index(char c)
    {
        Ensures((c >= 'A' and c <= 'Z') or
               (c >= 'a' and c <= 'z') or
               (c >= '0' and c <= '9') or
                c == '+' or c == '/' or c == '=');

        if(c == '=') return 64;
        if(c >= 'a') return ('Z'-'A') + 1 + (c - 'a');
        if(c >= 'A') return (c - 'A');
        if(c >= '0') return ('Z'-'A') + ('z'-'a') + 2 + (c - '0');
        if(c == '+') return 62;
        if(c == '/') return 63;
        return 64;
    }

    inline std::string encode(std::span<const std::byte> source)
    {
        auto encoded = std::string{};

        while(source.size() > 0)
        {
            auto index1 = (source[0] bitand std::byte{0b11111100}) >> 2,
                 index2 = (source[0] bitand std::byte{0b00000011}) << 4,
                 index3 = std::byte{64},
                 index4 = std::byte{64};

            if(source.size() > 1)
            {
                index2 |= (source[1] bitand std::byte{0b11110000}) >> 4;
                index3  = (source[1] bitand std::byte{0b00001111}) << 2;
            }

            if(source.size() > 2)
            {
                index3 |= (source[2] bitand std::byte{0b11000000}) >> 6;
                index4  = (source[2] bitand std::byte{0b00111111});
            }

            encoded.push_back(to_character_set(index1));
            encoded.push_back(to_character_set(index2));
            encoded.push_back(to_character_set(index3));
            encoded.push_back(to_character_set(index4));

            source = source.subspan(std::min(3ul,source.size()));
        }

        return encoded;
    }

    inline std::string encode(std::string_view source)
    {
        return encode(std::as_bytes(std::span{source.cbegin(),source.cend()}));
    }

    inline std::string decode(std::string_view source)
    {
        auto decoded = std::string{};

        while(source.size() > 3)
        {
            auto index1 = to_index(source[0]),
                 index2 = to_index(source[1]),
                 index3 = to_index(source[2]),
                 index4 = to_index(source[3]);

            decoded.push_back(static_cast<char>((index1 << 2) | (index2 >> 4)));

            if(index3 < 64)
                decoded.push_back(static_cast<char>((index2 << 4) | (index3 >> 2)));

            if(index4 < 64)
                decoded.push_back(static_cast<char>((index3 << 6) | index4));

            source.remove_prefix(std::min(4ul,source.size()));
        }
        return decoded;
    }

} // namespace http::base64
