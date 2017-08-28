/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _CRYPTO_SHA1_H_
#define	_CRYPTO_SHA1_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define	SHA_DIGEST_LENGTH	20
#define	SHA1_RESULTLEN		SHA_DIGEST_LENGTH

typedef struct sha1_ctxt {
	union {
		u_int8_t	b8[20];
		u_int32_t	b32[5];	/* state (ABCDE) */
	} h;
	union {
		u_int8_t	b8[8];
		u_int32_t	b32[2];
		u_int64_t	b64[1];	/* # of bits, modulo 2^64 (msb first) */
	} c;
	union {
		u_int8_t	b8[64];
		u_int32_t	b32[16]; /* input buffer */
	} m;
	u_int8_t	count;		/* unused; for compatibility only */
} SHA1_CTX;

/* For compatibility with the other SHA-1 implementation. */
#define sha1_init(c)		SHA1Init(c)
#define sha1_loop(c, b, l)	SHA1Update(c, b, l)
#define sha1_result(c, b)	SHA1Final(b, c)

extern void SHA1Init(SHA1_CTX *);
extern void SHA1Update(SHA1_CTX *, const void *, size_t);
extern void SHA1UpdateUsePhysicalAddress(SHA1_CTX *, const void *, size_t);
extern void SHA1Final(void *, SHA1_CTX *);

#ifdef  __cplusplus
}
#endif

#include <array>
#include <algorithm>
#include "http/base64.hpp"
#include "gsl/assert.hpp"

namespace sha1 {

using namespace std::literals;

static unsigned char result[SHA1_RESULTLEN];

inline const std::byte* binary(std::string_view source)
{
    auto ctx = sha1_ctxt{};
    sha1_init(&ctx);
    sha1_loop(&ctx, reinterpret_cast<const uint8_t*>(source.data()), source.length());
    sha1_result(&ctx, &result);
    return reinterpret_cast<const std::byte*>(result);
}

inline std::string base64(std::string_view source)
{
    auto tmp1 = binary(source);
    auto tmp2 = std::basic_string_view<unsigned char>{reinterpret_cast<const unsigned char*>(tmp1), SHA1_RESULTLEN};
    return http::base64::encode(tmp2);
}

inline void Encode(unsigned char *output, u_int32_t *input, unsigned int len)
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j + 3] = input[i] & 0xff;
		output[j + 2] = (input[i] >> 8) & 0xff;
		output[j + 1] = (input[i] >> 16) & 0xff;
		output[j] = (input[i] >> 24) & 0xff;
	}
}

class sha_1
{
    union
    {
        std::uint32_t h[5];
        std::aligned_storage_t<160> message_digest;
    };

    union
    {
        std::uint64_t ml;
        std::aligned_storage_t<64> message_length;
        char c[8];
    };

    template<typename Chunk>
    void transform(const Chunk& chunk)
    {
        auto w = std::array<std::uint32_t,80>{};

        for(auto i = 0u; i < 16u; ++i)
            w[i] = (chunk[i] << 24) xor (chunk[i+1] << 16) xor (chunk[i+2] << 8) xor chunk[i+3];

        for(auto i = 16u; i < 32u; ++i)
            w[i] = (w[i-3] xor w[i-8] xor w[i-14] xor w[i-16]) << 1;

        for(auto i = 32u; i < 79u; ++i)
            w[i] = (w[i-6] xor w[i-16] xor w[i-28] xor w[i-32]) << 2;

        auto a = h[0],
             b = h[1],
             c = h[2],
             d = h[3],
             e = h[4],
             f = 0u,
             k = 0u;

        for(auto i = 0ul; i < 79ul; ++i)
        {
            if (i <= 19)
            {
                f = (b and c) or ((not b) and d);
                k = 0x5A827999u;
            }
            else if (20 <= i and i <= 39)
            {
                f = b xor c xor d;
                k = 0x6ED9EBA1u;
            }
            else if (40 <= i and i <= 59)
            {
                f = (b and c) or (b and d) or (c and d);
                k = 0x8F1BBCDCu;
            }
            else if (60 <= i and i <= 79)
            {
                f = b xor c xor d;
                k = 0xCA62C1D6u;
            }
            auto temp = (a << 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30);
            b = a;
            a = temp;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }

public:

    sha_1() :
        h{0x67452301u,
          0xEFCDAB89u,
          0x98BADCFEu,
          0x10325476u,
          0xC3D2E1F0u},
        ml{0ll}
    {}

    void append(std::string_view message)
    {
        if(message.length() == 0)
            return;

        ml += 8 * message.length();

        while(!message.empty())
        {
            const auto chunk = message.substr(0, std::min(message.length(), 64ul));
            message.remove_prefix(chunk.length());
            if(chunk.length() == 64ul)
            {
                transform(chunk);
            }
            else
            {
                auto buffer = std::array<char,64>{};
                std::fill(buffer.begin(), buffer.end(), '\x00');
                auto itr = std::copy(chunk.cbegin(), chunk.cend(), buffer.begin());
                *itr = '\x80';

                static unsigned char bits[8];
                Encode(bits, reinterpret_cast<std::uint32_t*>(&message_digest), 8);
                std::copy_n(bits, 8, &buffer[56-1]);

                transform(buffer);
            }
        }
    }

    const std::byte* data() noexcept
    {
        static unsigned char buffer[20];
        Encode(buffer, reinterpret_cast<std::uint32_t*>(&message_digest), 20);
        return reinterpret_cast<const std::byte*>(&buffer);

        // return reinterpret_cast<std::byte*>(&message_digest);
    }

    constexpr std::size_t size() noexcept
    {
        return 20ul;
    }
};

} // namespace sha1

#endif /*_CRYPTO_SHA1_H_*/
