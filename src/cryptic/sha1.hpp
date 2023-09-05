// Copyright (c) 2017 Kaius Ruokonen
// Distributed under the MIT software license
// See LICENCE file or https://opensource.org/licenses/MIT

#pragma once
#include <bit>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include "gsl/narrow_cast.hpp"
#include "cryptic/base64.hpp"

namespace cryptic {

class sha1
{
public:

    using message_length_type = std::uint64_t;

    using buffer_type = std::array<std::byte,20>;

    constexpr sha1() noexcept :
        m_message_length{0ull},
        m_message_digest{0x67452301u,
                         0xEFCDAB89u,
                         0x98BADCFEu,
                         0x10325476u,
                         0xC3D2E1F0u}
    {}

    sha1(std::span<const std::byte> message) noexcept : sha1()
    {
        hash(message);
    }

    sha1(std::string_view message) noexcept : sha1()
    {
        hash(message);
    }

    void hash(std::span<const std::byte> message) noexcept
    {
        reset();
        while(message.size() >= 64)
        {
            update(message.first<64>());
            message = message.subspan<64>();
        }
        finalize(message);
    }

    void hash(std::string_view message) noexcept
    {
        hash(std::as_bytes(std::span{message.cbegin(),message.cend()}));
    }

    void encode(std::span<std::byte,20> other) const noexcept
    {
        const auto bytes = std::as_bytes(std::span{m_message_digest});
    	for(auto i = std::uint_fast8_t{0u}; i < 20u; i += 4u)
        {
    		other[i+0] = bytes[i+3];
    		other[i+1] = bytes[i+2];
    		other[i+2] = bytes[i+1];
    		other[i+3] = bytes[i+0];
        }
    }

    std::string base64() const
    {
        auto buffer = buffer_type{};
        encode(buffer);
        return base64::encode(buffer);
    }

    static std::string base64(std::span<const std::byte> message)
    {
        const auto hash = sha1{message};
        return hash.base64();
    }

    static std::string base64(std::string_view message)
    {
        const auto hash = sha1{message};
        return hash.base64();
    }

    std::string hexadecimal() const
    {
        auto ss = std::stringstream{};
        ss << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[0u]
           << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[1u]
           << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[2u]
           << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[3u]
           << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[4u];
        return ss.str();
    }

    static std::string hexadecimal(std::span<const std::byte> message)
    {
        const auto hash = sha1{message};
        return hash.hexadecimal();
    }

    static std::string hexadecimal(std::string_view message)
    {
        const auto hash = sha1{message};
        return hash.hexadecimal();
    }

    constexpr std::size_t size() const noexcept
    {
        return 20ul;
    }

    bool operator < (const sha1& other) const noexcept
    {
    	for(auto i = std::uint_fast8_t{0u}; i < sizeof m_message_digest; ++i)
            if(m_message_digest[i] != other.m_message_digest[i])
                return m_message_digest[i] < other.m_message_digest[i];
        return false;
    }

    bool operator < (std::span<const std::byte, 20> other) const noexcept
    {
        const auto bytes = std::as_bytes(std::span{m_message_digest});
    	for(auto i = std::uint_fast8_t{0u}; i < other.size(); i += 4u)
        {
    		if(bytes[i+3] != other[i+0]) return bytes[i+3] < other[i+0];
    		if(bytes[i+2] != other[i+1]) return bytes[i+2] < other[i+1];
    		if(bytes[i+1] != other[i+2]) return bytes[i+1] < other[i+2];
    		if(bytes[i+0] != other[i+3]) return bytes[i+0] < other[i+3];
        }
        return false;
        return true;
    }

private:

    constexpr void reset()
    {
        m_message_length = 0ull;
        m_message_digest[0] = 0x67452301u;
        m_message_digest[1] = 0xEFCDAB89u;
        m_message_digest[2] = 0x98BADCFEu;
        m_message_digest[3] = 0x10325476u;
        m_message_digest[4] = 0xC3D2E1F0u;
    }

    void update(std::span<const std::byte,64> chunk) noexcept
    {
        m_message_length += 8ull * 64ull; // NOTE, bits
        transform(chunk.data());
    }

    void finalize(std::span<const std::byte> chunk)
    {
        Expects(chunk.size() < 64);
        m_message_length += 8ull * chunk.size(); // NOTE, bits
        auto temp = std::array<std::byte,64>{};
        temp.fill(std::byte{0b00000000});
        auto itr = std::copy(chunk.begin(), chunk.end(), temp.begin());
        *itr++ =  std::byte{0b10000000};
        if(std::distance(itr, temp.end()) < 8)
        {
            transform(temp.data());
            temp.fill(std::byte{0b00000000});
        }
        auto length = std::span{temp}.last<8>();
        encode(length.data(), m_message_length);
        transform(temp.data());
    }

    constexpr void transform(const std::byte* chunk) noexcept
    {
        Expects(chunk);
        auto words = std::array<std::uint32_t,80>{};

        for(auto i = std::uint_fast8_t{0u}, j = std::uint_fast8_t{0u}; i < 16u; ++i, j += 4u)
            words[i] = std::to_integer<uint32_t>(chunk[j+0]) << 24 xor
                       std::to_integer<uint32_t>(chunk[j+1]) << 16 xor
                       std::to_integer<uint32_t>(chunk[j+2]) <<  8 xor
                       std::to_integer<uint32_t>(chunk[j+3]);

        for(auto i = std::uint_fast8_t{16u}; i < 32u; ++i)
            words[i] = std::rotl(words[i-3] xor words[i-8] xor words[i-14] xor words[i-16], 1);

        for(auto i = std::uint_fast8_t{32u}; i < 80u; ++i)
            words[i] = std::rotl(words[i-6] xor words[i-16] xor words[i-28] xor words[i-32], 2);

        auto a = m_message_digest[0],
             b = m_message_digest[1],
             c = m_message_digest[2],
             d = m_message_digest[3],
             e = m_message_digest[4],
             f = 0u,
             k = 0u;

        for(auto i = std::uint_fast8_t{0u}; i < 20u; ++i)
        {
            f = (b bitand c) bitor ((compl b) bitand d);
            k = 0x5A827999u;
            auto temp = std::rotl(a,5) + f + e + k + words[i];
            e = d;
            d = c;
            c = std::rotl(b,30);
            b = a;
            a = temp;
        }

        for(auto i = std::uint_fast8_t{20u}; i < 40u; ++i)
        {
            f = b xor c xor d;
            k = 0x6ED9EBA1u;
            auto temp = std::rotl(a,5) + f + e + k + words[i];
            e = d;
            d = c;
            c = std::rotl(b,30);
            b = a;
            a = temp;
        }

        for(auto i = std::uint_fast8_t{40u}; i < 60u; ++i)
        {
            f = (b bitand c) bitor (b bitand d) bitor (c bitand d);
            k = 0x8F1BBCDCu;
            auto temp = std::rotl(a,5) + f + e + k + words[i];
            e = d;
            d = c;
            c = std::rotl(b,30);
            b = a;
            a = temp;
        }

        for(auto i = std::uint_fast8_t{60u}; i < 80u; ++i)
        {
            f = b xor c xor d;
            k = 0xCA62C1D6u;
            auto temp = std::rotl(a,5) + f + e + k + words[i];
            e = d;
            d = c;
            c = std::rotl(b,30);
            b = a;
            a = temp;
        }

        m_message_digest[0] += a;
        m_message_digest[1] += b;
        m_message_digest[2] += c;
        m_message_digest[3] += d;
        m_message_digest[4] += e;
    }

    static constexpr void encode(std::byte* output, const message_length_type length) noexcept
    {
        Expects(output);
    	output[7] = gsl::narrow_cast<std::byte>(length >>  0);
    	output[6] = gsl::narrow_cast<std::byte>(length >>  8);
    	output[5] = gsl::narrow_cast<std::byte>(length >> 16);
    	output[4] = gsl::narrow_cast<std::byte>(length >> 24);
    	output[3] = gsl::narrow_cast<std::byte>(length >> 32);
    	output[2] = gsl::narrow_cast<std::byte>(length >> 40);
    	output[1] = gsl::narrow_cast<std::byte>(length >> 48);
    	output[0] = gsl::narrow_cast<std::byte>(length >> 56);
    }

    message_length_type m_message_length;

    std::array<std::uint32_t,5> m_message_digest;
};

} // namespace cryptic
