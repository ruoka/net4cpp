#pragma once
#include <vector>
#include "gsl/assert.hpp"

namespace ws {

constexpr std::uint16_t continuation = 0b0000; // 0
constexpr std::uint16_t text         = 0b0001; // 1
constexpr std::uint16_t binary       = 0b0010; // 2
constexpr std::uint16_t close        = 0b1000; // 8
constexpr std::uint16_t ping         = 0b1001; // 9
constexpr std::uint16_t pong         = 0b1010; // 10

struct frame
{
    union alignas(16)
    {
        struct alignas(16)
        {
            std::uint16_t opcode : 4;
            std::uint16_t rsv3 : 1;
            std::uint16_t rsv1 : 1;
            std::uint16_t rsv2 : 1;
            std::uint16_t fin : 1;
            std::uint16_t payload_length : 7;
            std::uint16_t masked : 1;
        } header;
        std::uint16_t bits;
    };
    std::uint16_t extended_payload_length_16; // NOTE, optional
    std::uint64_t extended_payload_length_64; // NOTE, optional
    union alignas(32)
    {
        std::uint32_t masking_key;
        std::uint8_t masking_key_octet[4];
    };
    std::vector<char> extension_data; // NOTE, optional
    std::vector<char> payload_data; // NOTE, optional
};

std::size_t length(const frame& f) noexcept
{
    return std::max<std::size_t>({f.header.payload_length,f.extended_payload_length_16,f.extended_payload_length_64});
}

} // namespace ws

namespace std
{

template<typename Byte>
auto& operator << (std::basic_ostream<Byte>& os, const ws::frame& f)
{
    Expects((f.header.payload_length != 126 && f.header.payload_length != 127) ||
            (f.header.payload_length == 126 && f.extended_payload_length_16)  ||
            (f.header.payload_length == 127 && f.extended_payload_length_64)  );
    Expects((!f.header.masked && !f.masking_key) || (f.header.masked&& f.masking_key));
    os.write(reinterpret_cast<const Byte*>(&f.bits), 2);
    if(f.extended_payload_length_16)
        os.write(reinterpret_cast<const Byte*>(&f.extended_payload_length_16), 2);
    if(f.extended_payload_length_64)
        os.write(reinterpret_cast<const Byte*>(&f.extended_payload_length_64), 8);
    if(f.header.masked)
        os.write(reinterpret_cast<const Byte*>(&f.masking_key), 4);
    for (const auto c : f.extension_data)
        os.put(static_cast<Byte>(c));
    for (const auto c : f.payload_data)
        os.put(static_cast<Byte>(c));
    return os;
}

template<typename Byte>
auto& operator >> (std::basic_istream<Byte>& is, ws::frame& f)
{
    is.read(reinterpret_cast<Byte*>(&f.header), 2);
    if(f.header.payload_length == 126)
        is.read(reinterpret_cast<Byte*>(&f.extended_payload_length_16), 2);
    else if(f.header.payload_length == 127)
        is.read(reinterpret_cast<Byte*>(&f.extended_payload_length_64), 8);
    if(f.header.masked)
        is.read(reinterpret_cast<Byte*>(&f.masking_key), 4);
    for(auto i = 0, j = 0; i < length(f); ++i, j = i % 4)
        f.payload_data.push_back(is.get() xor f.masking_key_octet[j]);
    Ensures((f.header.payload_length != 126 && f.header.payload_length != 127) ||
            (f.header.payload_length == 126 && f.extended_payload_length_16)   ||
            (f.header.payload_length == 127 && f.extended_payload_length_64));
    Ensures((!f.header.masked && !f.masking_key) || (f.header.masked && f.masking_key));
    return is;
}

} // namespace ws
