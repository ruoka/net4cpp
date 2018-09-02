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

template<uint32_t H0, uint32_t H1, uint32_t H2, uint32_t H3, uint32_t H4, uint32_t H5, uint32_t H6, uint32_t H7, size_t N>
class sha2
{
public:

    using size_type = std::uint64_t;

    sha2() noexcept :
        m_message_length{0ull},
        m_message_digest{H0,H1,H2,H3,H4,H5,H6,H7}
    {}

    sha2(span<const byte> message) noexcept : sha2()
    {
        hash(message);
    }

    void hash(span<const byte> message) noexcept
    {
        m_message_digest[0] = H0;
        m_message_digest[1] = H1;
        m_message_digest[2] = H2;
        m_message_digest[3] = H3;
        m_message_digest[4] = H4;
        m_message_digest[5] = H5;
        m_message_digest[6] = H6;
        m_message_digest[7] = H7;
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
        auto hash = sha2{message};
        return hash.base64();
    }

    string hexadecimal()
    {
        auto ss = stringstream{};
        for(auto i = 0u; i < N; ++i)
            ss << setw(8) << setfill('0') << hex << m_message_digest[i];
        return ss.str();
    }

    static string hexadecimal(span<const byte> message)
    {
        auto hash = sha2{message};
        return hash.hexadecimal();
    }

private:

    template<size_t Rotation, typename Unsigned>
    static constexpr Unsigned rightrotate(Unsigned number)
    {
        static_assert(is_unsigned_v<Unsigned>);
        constexpr auto bits = numeric_limits<Unsigned>::digits;
        static_assert(Rotation <= bits);
        return (number >> Rotation) bitor (number << (bits - Rotation));
    }

    void transform(span<const byte, 64> chunk) noexcept
    {
        auto words = array<uint32_t,64>{};

        for(auto i = 0u, j = 0u; i < 16u; ++i, j += 4u)
            words[i] = to_integer<uint32_t>(chunk[j+0]) << 24 xor
                       to_integer<uint32_t>(chunk[j+1]) << 16 xor
                       to_integer<uint32_t>(chunk[j+2]) <<  8 xor
                       to_integer<uint32_t>(chunk[j+3]);

        for(auto i = 16u; i < 64u; ++i)
        {
            const auto s0 = rightrotate<7>(words[i-15]) xor rightrotate<18>(words[i-15]) xor (words[i-15] >> 3);
            const auto s1 = rightrotate<17>(words[i-2]) xor rightrotate<19>(words[i-2]) xor (words[i-2] >> 10);
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

        for(auto i = 0u; i < 64u; ++i)
        {
            const auto S1 = rightrotate<6>(e) xor rightrotate<11>(e) xor rightrotate<25>(e);
            const auto ch = (e bitand f) xor ((compl e) bitand g);
            const auto temp1 = h + S1 + ch + k[i] + words[i];
            const auto S0 = rightrotate<2>(a) xor rightrotate<13>(a) xor rightrotate<22>(a);
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

    static constexpr array<uint32_t,64> k =
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

    array<uint32_t,8> m_message_digest;

    array<byte,4*N> m_buffer;
};

using sha224 = sha2<0xc1059ed8u,0x367cd507u,0x3070dd17u,0xf70e5939u,0xffc00b31u,0x68581511u,0x64f98fa7u,0xbefa4fa4u,7>;

using sha256 = sha2<0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u,8>;

} // namespace cryptic
