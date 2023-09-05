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

template<std::uint32_t H0, std::uint32_t H1, std::uint32_t H2, std::uint32_t H3, std::uint32_t H4, std::uint32_t H5, std::uint32_t H6, std::uint32_t H7, std::size_t N>
class sha2
{
public:

    using message_length_type = std::uint64_t;

    using buffer_type = std::array<std::byte, 4 * N>;

    constexpr sha2() noexcept :
        m_message_length{0ull},
        m_message_digest{H0,H1,H2,H3,H4,H5,H6,H7}
    {}

    sha2(std::span<const std::byte> message) noexcept : sha2()
    {
        hash(message);
    }

    sha2(std::string_view message) noexcept : sha2()
    {
        hash(message);
    }

    sha2(const sha2&) = default;

    sha2(sha2&&) = default;

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

    void encode(std::span<std::byte, 4 * N> other) const noexcept
    {
        const auto bytes = std::as_bytes(std::span{m_message_digest});
    	for(auto i = std::uint_fast8_t{0u}; i < other.size(); i += 4u)
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
        const auto hash = sha2{message};
        return hash.base64();
    }

    static std::string base64(std::string_view message)
    {
        const auto hash = sha2{message};
        return hash.base64();
    }

    std::string hexadecimal() const
    {
        auto ss = std::stringstream{};
        for(auto i = std::uint_fast8_t{0u}; i < N; ++i)
            ss << std::setw(8) << std::setfill('0') << std::hex << m_message_digest[i];
        return ss.str();
    }

    static std::string hexadecimal(std::span<const std::byte> message)
    {
        const auto hash = sha2{message};
        return hash.hexadecimal();
    }

    static std::string hexadecimal(std::string_view message)
    {
        const auto hash = sha2{message};
        return hash.hexadecimal();
    }

    constexpr std::size_t size() const noexcept
    {
        return 4 * N;
    }

    bool operator < (const sha2& other) const noexcept
    {
    	for(auto i = std::uint_fast8_t{0u}; i < N; ++i)
            if(m_message_digest[i] != other.m_message_digest[i])
                return m_message_digest[i] < other.m_message_digest[i];
        return false;
    }

    bool operator < (std::span<const std::byte, 4 * N> other) const noexcept
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
    }

private:

    constexpr void reset()
    {
        m_message_length = 0ull;
        m_message_digest[0] = H0;
        m_message_digest[1] = H1;
        m_message_digest[2] = H2;
        m_message_digest[3] = H3;
        m_message_digest[4] = H4;
        m_message_digest[5] = H5;
        m_message_digest[6] = H6;
        m_message_digest[7] = H7;
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
        *itr++ = std::byte{0b10000000};
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
        auto words = std::array<std::uint32_t,64>{};

        for(auto i = std::uint_fast8_t{0u}, j = std::uint_fast8_t{0u}; i < 16u; ++i, j += 4u)
            words[i] = std::to_integer<std::uint32_t>(chunk[j+0]) << 24 xor
                       std::to_integer<std::uint32_t>(chunk[j+1]) << 16 xor
                       std::to_integer<std::uint32_t>(chunk[j+2]) <<  8 xor
                       std::to_integer<std::uint32_t>(chunk[j+3]) <<  0;

        for(auto i = std::uint_fast8_t{16u}; i < 64u; ++i)
        {
            const auto s0 = std::rotr(words[i-15], 7) xor std::rotr(words[i-15], 18) xor (words[i-15] >> 3);
            const auto s1 = std::rotr(words[i-2], 17) xor std::rotr(words[i-2], 19) xor (words[i-2] >> 10);
            words[i] = words[i-16] + s0 + words[i-7] + s1;
        }

        auto a = m_message_digest[0],
             b = m_message_digest[1],
             c = m_message_digest[2],
             d = m_message_digest[3],
             e = m_message_digest[4],
             f = m_message_digest[5],
             g = m_message_digest[6],
             h = m_message_digest[7];

        for(auto i = std::uint_fast8_t{0u}; i < 64u; ++i)
        {
            const auto S1 = std::rotr(e,6) xor std::rotr(e,11) xor std::rotr(e,25);
            const auto ch = (e bitand f) xor ((compl e) bitand g);
            const auto temp1 = h + S1 + ch + k[i] + words[i];
            const auto S0 = std::rotr(a,2) xor std::rotr(a,13) xor std::rotr(a,22);
            const auto maj = (a bitand b) xor (a bitand c) xor (b bitand c);
            const auto temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        m_message_digest[0] += a;
        m_message_digest[1] += b;
        m_message_digest[2] += c;
        m_message_digest[3] += d;
        m_message_digest[4] += e;
        m_message_digest[5] += f;
        m_message_digest[6] += g;
        m_message_digest[7] += h;
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

    static constexpr std::array<std::uint32_t,64> k =
    {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };

    message_length_type m_message_length;

    std::array<std::uint32_t,8> m_message_digest;
};

using sha224 = sha2<0xc1059ed8u,0x367cd507u,0x3070dd17u,0xf70e5939u,0xffc00b31u,0x68581511u,0x64f98fa7u,0xbefa4fa4u,7>;

using sha256 = sha2<0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u,8>;

} // namespace cryptic
