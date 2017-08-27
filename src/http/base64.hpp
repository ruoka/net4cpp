#pragma once
#include <string>
#include <algorithm>

namespace http::base64 {

    inline constexpr char to_character_set(std::size_t i)
    {
        assert(i < 65);
        constexpr char set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
        return set[i];
    }

    inline constexpr char to_index(char c)
    {
        assert((c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') ||
                c == '+' || c == '/' || c == '=');

        if(c == '=') return 64;
        if(c >= 'a') return ('Z'-'A') + 1 + (c - 'a');
        if(c >= 'A') return (c - 'A');
        if(c >= '0') return ('Z'-'A') + ('z'-'a') + 2 + (c - '0');
        if(c == '+') return 62;
        if(c == '/') return 63;
        return 64;
    }

    inline std::string encode(std::basic_string_view<unsigned char> source)
    {
        auto encoded = std::string{};

        while(source.size() > 0)
        {
            auto index1 = (source[0] & 0b11111100) >> 2,
                 index2 = (source[0] & 0b00000011) << 4,
                 index3 = 64,
                 index4 = 64;

            if(source.size() > 1)
            {
                index2 |= (source[1] & 0b11110000) >> 4;
                index3  = (source[1] & 0b00001111) << 2;
            }

            if(source.size() > 2)
            {
                index3 |= (source[2] & 0b11000000) >> 6;
                index4  = (source[2] & 0b00111111);
            }

            encoded.push_back(to_character_set(index1));
            encoded.push_back(to_character_set(index2));
            encoded.push_back(to_character_set(index3));
            encoded.push_back(to_character_set(index4));

            source.remove_prefix(std::min(3ul,source.size()));
        }

        return encoded;
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

            decoded.push_back((index1 << 2) | (index2 >> 4));

            if(index3 < 64)
                decoded.push_back((index2 << 4) | (index3 >> 2));

            if(index4 < 64)
                decoded.push_back((index3 << 6) | index4);

            source.remove_prefix(std::min(4ul,source.size()));
        }
        return decoded;
    }

} // namespace http::base64
