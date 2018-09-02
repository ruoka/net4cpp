// Copyright (c) 2017 Kaius Ruokonen
// Distributed under the MIT software license
// See LICENCE file or https://opensource.org/licenses/MIT

#pragma once
#include <array>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "gsl/span.hpp"
#include "cryptic/base64.hpp"

namespace cryptic {

using namespace std;
using namespace gsl;

class sha1
{
public:

    using size_type = std::uint64_t;

    sha1() noexcept :
        m_message_length{0ull},
        m_message_digest{0x67452301u,
                         0xEFCDAB89u,
                         0x98BADCFEu,
                         0x10325476u,
                         0xC3D2E1F0u},
        m_buffer{}
    {}

    sha1(span<const byte> message) noexcept : sha1()
    {
        hash(message);
    }

    void hash(span<const byte> message) noexcept
    {
        m_message_digest[0] = 0x67452301u;
        m_message_digest[1] = 0xEFCDAB89u;
        m_message_digest[2] = 0x98BADCFEu;
        m_message_digest[3] = 0x10325476u;
        m_message_digest[4] = 0xC3D2E1F0u;
        m_message_length += 8ull * static_cast<size_type>(message.size());

        while(message.size() >= 64)
        {
            const auto chunk = message.first<64>();
            transform(chunk);
            message = message.subspan<64>();
        }

        auto chunk = array<byte,64>{};
        auto itr = copy(message.cbegin(), message.cend(), chunk.begin());
        *itr++ = byte{0b10000000};
        fill(itr, chunk.end(), byte{0b00000000});

        if(distance(itr, chunk.end()) < 8)
        {
            transform(chunk);
            fill_n(chunk.begin(), 56, byte{0b00000000});
        }

        auto length = make_span(chunk).last<8>();
        encode(length, m_message_length);
        transform(chunk);
    }

    const byte* data() noexcept
    {
        encode(m_buffer, m_message_digest);
        return m_buffer.data();
    }

    constexpr size_type size() const
    {
        return m_buffer.size();
    }

    string base64()
    {
        data();
        return base64::encode(m_buffer);
    }

    static string base64(span<const byte> message)
    {
        auto hash = sha1{message};
        return hash.base64();
    }

    string hexadecimal()
    {
        auto ss = stringstream{};
        ss << setw(8) << setfill('0') << hex << m_message_digest[0u]
           << setw(8) << setfill('0') << hex << m_message_digest[1u]
           << setw(8) << setfill('0') << hex << m_message_digest[2u]
           << setw(8) << setfill('0') << hex << m_message_digest[3u]
           << setw(8) << setfill('0') << hex << m_message_digest[4u];
        return ss.str();
    }

    static string hexadecimal(span<const byte> message)
    {
        auto hash = sha1{message};
        return hash.hexadecimal();
    }

private:

    template<size_t Rotation, typename Unsigned>
    static constexpr Unsigned leftrotate(Unsigned number)
    {
        static_assert(is_unsigned_v<Unsigned>);
        constexpr auto bits = numeric_limits<Unsigned>::digits;
        static_assert(Rotation <= bits);
        return (number << Rotation) bitor (number >> (bits - Rotation));
    }

    void transform(span<const byte, 64> chunk) noexcept
    {
        auto words = array<uint32_t,80>{};

        for(auto i = 0u, j = 0u; i < 16u; ++i, j += 4u)
            words[i] = to_integer<uint32_t>(chunk[j+0]) << 24 xor
                       to_integer<uint32_t>(chunk[j+1]) << 16 xor
                       to_integer<uint32_t>(chunk[j+2]) <<  8 xor
                       to_integer<uint32_t>(chunk[j+3]);

        for(auto i = 16u; i < 32u; ++i)
            words[i] = leftrotate<1>(words[i-3] xor words[i-8] xor words[i-14] xor words[i-16]);

        for(auto i = 32u; i < 80u; ++i)
            words[i] = leftrotate<2>(words[i-6] xor words[i-16] xor words[i-28] xor words[i-32]);

        auto a = m_message_digest[0],
             b = m_message_digest[1],
             c = m_message_digest[2],
             d = m_message_digest[3],
             e = m_message_digest[4],
             f = 0u,
             k = 0u;

        for(auto i = 0u; i < 20u; ++i)
        {
            f = (b bitand c) bitor ((compl b) bitand d);
            k = 0x5A827999u;
            auto temp = leftrotate<5>(a) + f + e + k + words[i];
            e = d;
            d = c;
            c = leftrotate<30>(b);
            b = a;
            a = temp;
        }

        for(auto i = 20u; i < 40u; ++i)
        {
            f = b xor c xor d;
            k = 0x6ED9EBA1u;
            auto temp = leftrotate<5>(a) + f + e + k + words[i];
            e = d;
            d = c;
            c = leftrotate<30>(b);
            b = a;
            a = temp;
        }

        for(auto i = 40u; i < 60u; ++i)
        {
            f = (b bitand c) bitor (b bitand d) bitor (c bitand d);
            k = 0x8F1BBCDCu;
            auto temp = leftrotate<5>(a) + f + e + k + words[i];
            e = d;
            d = c;
            c = leftrotate<30>(b);
            b = a;
            a = temp;
        }

        for(auto i = 60u; i < 80u; ++i)
        {
            f = b xor c xor d;
            k = 0xCA62C1D6u;
            auto temp = leftrotate<5>(a) + f + e + k + words[i];
            e = d;
            d = c;
            c = leftrotate<30>(b);
            b = a;
            a = temp;
        }

        m_message_digest[0] += a;
        m_message_digest[1] += b;
        m_message_digest[2] += c;
        m_message_digest[3] += d;
        m_message_digest[4] += e;
    }

    template<typename Type, typename Integer>
    static constexpr byte narrow(Integer number)
    {
        static_assert(is_integral_v<Integer>);
        static_assert(numeric_limits<Type>::digits < numeric_limits<Integer>::digits);
        return static_cast<Type>(number bitand 0b11111111);
    }

    static void encode(span<byte> output, const size_type length) noexcept
    {
    	output[7] = narrow<byte>(length >>  0);
    	output[6] = narrow<byte>(length >>  8);
    	output[5] = narrow<byte>(length >> 16);
    	output[4] = narrow<byte>(length >> 24);
    	output[3] = narrow<byte>(length >> 32);
    	output[2] = narrow<byte>(length >> 40);
    	output[1] = narrow<byte>(length >> 48);
    	output[0] = narrow<byte>(length >> 56);
    }

    static void encode(span<byte> output, const span<uint32_t> input) noexcept
    {
    	for(auto i = 0, j = 0; j < output.size(); ++i, j += 4)
        {
    		output[j+3] = narrow<byte>(input[i]);
    		output[j+2] = narrow<byte>(input[i] >>  8);
    		output[j+1] = narrow<byte>(input[i] >> 16);
    		output[j+0] = narrow<byte>(input[i] >> 24);
    	}
    }

    size_type m_message_length;

    array<uint32_t,5> m_message_digest;

    array<byte,20> m_buffer;
};

} // namespace cryptic
