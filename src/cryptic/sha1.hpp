#pragma once
#include <array>
#include <sstream>
#include <algorithm>
#include "gsl/span.hpp"
#include "gsl/assert.hpp"
#include "cryptic/base64.hpp"

namespace cryptic {

using namespace std;
using namespace gsl;

class sha1
{
public:

    sha1() noexcept :
        message_digest{0x67452301u,
                       0xEFCDAB89u,
                       0x98BADCFEu,
                       0x10325476u,
                       0xC3D2E1F0u},
        buffer{},
        message_length{0ull}
    {}

    sha1(span<const byte> message) : sha1()
    {
        hash(message);
    }

    void hash(span<const byte> message)
    {
        message_digest = {0x67452301u,
                          0xEFCDAB89u,
                          0x98BADCFEu,
                          0x10325476u,
                          0xC3D2E1F0u};

        message_length += 8 * message.size();

        while(message.size() >= 64)
        {
            const auto chunk = message.subspan(0, 64);
            transform(chunk);
            message = message.subspan<64>();
        }

        auto chunk = array<byte,64>{};
        auto itr = copy(message.cbegin(), message.cend(), chunk.begin());
        *itr++ = byte{0b10000000};
        fill(itr, chunk.end(), byte{0b00000000});

        if(distance(chunk.begin(), itr) < 56)
        {
            auto length = make_span(chunk).subspan<56>();
            encode(length, message_length);
            transform(chunk);
        }
        else
        {
            transform(chunk);
            fill(chunk.begin(), itr, byte{0b00000000});
            auto length = make_span(chunk).subspan<56>();
            encode(length, message_length);
            transform(chunk);
        }
    }

    const byte* data() noexcept
    {
        encode(buffer, message_digest);
        return buffer.data();
    }

    constexpr size_t size() const noexcept
    {
        return buffer.size();
    }

    string base64()
    {
        return base64::encode(make_span(data(), size()));
    }

    static string base64(span<const byte> message)
    {
        auto hash = sha1{message};
        return hash.base64();
    }

    string hexadecimal()
    {
        auto ss = stringstream{};
        ss << setw(8) << setfill('0') << hex << message_digest[0]
           << setw(8) << setfill('0') << hex << message_digest[1]
           << setw(8) << setfill('0') << hex << message_digest[2]
           << setw(8) << setfill('0') << hex << message_digest[3]
           << setw(8) << setfill('0') << hex << message_digest[4];
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
        return (number << Rotation) bitor (number >> (bits-Rotation));
    }

    void transform(span<const byte> chunk) noexcept
    {
        Expects(chunk.size() == 64);

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

        auto a = message_digest[0],
             b = message_digest[1],
             c = message_digest[2],
             d = message_digest[3],
             e = message_digest[4],
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

        message_digest[0] += a;
        message_digest[1] += b;
        message_digest[2] += c;
        message_digest[3] += d;
        message_digest[4] += e;
    }

    template<typename Type, typename Integer>
    static constexpr byte narrow(Integer number)
    {
        static_assert(is_integral_v<Integer>);
        static_assert(numeric_limits<Type>::digits<numeric_limits<Integer>::digits);
        return static_cast<Type>(number bitand 0b11111111);
    }

    static void encode(span<byte> output, const uint64_t input) noexcept
    {
    	output[7] = narrow<byte>(input >>  0);
    	output[6] = narrow<byte>(input >>  8);
    	output[5] = narrow<byte>(input >> 16);
    	output[4] = narrow<byte>(input >> 24);
    	output[3] = narrow<byte>(input >> 32);
    	output[2] = narrow<byte>(input >> 40);
    	output[1] = narrow<byte>(input >> 48);
    	output[0] = narrow<byte>(input >> 56);
    }

    static void encode(span<byte> output, const span<uint32_t> input) noexcept
    {
    	for(auto i = 0ull, j = 0ull; j < output.size(); ++i, j += 4ull)
        {
    		output[j+3] = narrow<byte>(input[i]);
    		output[j+2] = narrow<byte>(input[i] >>  8);
    		output[j+1] = narrow<byte>(input[i] >> 16);
    		output[j+0] = narrow<byte>(input[i] >> 24);
    	}
    }

    array<uint32_t,5> message_digest;

    array<byte,20> buffer;

    uint64_t message_length;

};
} // namespace sha1
