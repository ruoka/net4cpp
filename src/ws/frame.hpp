#pragma once
#include <optional>
#include <vector>
#include "gsl/assert.hpp"

namespace ws {

struct frame
{
    static const std::byte continuation = std::byte{0x0};
    static const std::byte text = std::byte{0x1};
    static const std::byte binary = std::byte{0x2};
    static const std::byte close = std::byte{0x2};
    static const std::byte ping = std::byte{0x9};
    static const std::byte pong = std::byte{0xA};
    struct
    {
        std::byte fin : 1;
        std::byte rsv1 : 1;
        std::byte rsv2 : 1;
        std::byte rsv3 : 1;
        std::byte opcode : 4;
        std::byte masked : 1;
        std::byte payload_length : 7;
    } header;
    std::optional<std::uint16_t> extended_payload_length_16;
    std::optional<std::uint64_t> extended_payload_length_64;
    std::optional<std::uint32_t> masking_key;
    std::vector<std::byte> extension_data;
    std::vector<std::byte> payload_data;
};

} // namespace ws

namespace std
{

template<typename CharT>
auto& operator << (std::basic_ostream<CharT>& os, const ws::frame& f)
{
    Expects((f.header.payload_length != std::byte{126} && f.header.payload_length != std::byte{127}) ||
            (f.header.payload_length == std::byte{126} && f.extended_payload_length_16.has_value())  ||
            (f.header.payload_length == std::byte{127} && f.extended_payload_length_64.has_value())  );
    Expects((f.header.masked == std::byte{0b0} && !f.masking_key.has_value()) ||
            (f.header.masked == std::byte{0b1} && f.masking_key.has_value())  );

    os.write(reinterpret_cast<const CharT*>(&f.header), 2);
    if(f.extended_payload_length_16.has_value())
        os.write(reinterpret_cast<const CharT*>(&f.extended_payload_length_16.value()), 2);
    if(f.extended_payload_length_64.has_value())
        os.write(reinterpret_cast<const CharT*>(&f.extended_payload_length_64.value()), 8);
    if(f.masking_key.has_value())
        os.write(reinterpret_cast<const CharT*>(&f.masking_key.value()), 4);
    for (auto c : f.extension_data)
        os.put(static_cast<CharT>(c));
    for (auto c : f.payload_data)
        os.put(static_cast<CharT>(c));
    return os;
}

template<typename CharT>
auto& operator >> (std::basic_istream<CharT>& is, ws::frame& f)
{
    auto length = std::size_t{0};
    is.read(reinterpret_cast<CharT*>(&f.header), 2);
    if(f.header.payload_length == std::byte{126})
    {
        is.read(reinterpret_cast<CharT*>(&length), 2);
        f.extended_payload_length_16 = length;
    }
    else if(f.header.payload_length == std::byte{127})
    {
        is.read(reinterpret_cast<CharT*>(&length), 8);
        f.extended_payload_length_64 = length;
    }
    else
    {
        length = std::to_integer<std::size_t>(f.header.payload_length);
    }
    if(f.header.masked == std::byte{0b1})
    {
        auto key = std::uint32_t{};
        is.read(reinterpret_cast<CharT*>(&key), 4);
        f.masking_key = key;
    }
    while (length--)
        f.payload_data.push_back(std::byte(is.get()));

    Ensures((f.header.payload_length != std::byte{126} && f.header.payload_length != std::byte{127}) ||
            (f.header.payload_length == std::byte{126} && f.extended_payload_length_16.has_value())  ||
            (f.header.payload_length == std::byte{127} && f.extended_payload_length_64.has_value()) );
    Expects((f.header.masked == std::byte{0b0} && !f.masking_key.has_value()) ||
            (f.header.masked == std::byte{0b1} && f.masking_key.has_value())  );

    return is;
}

// Hack to get optional working with clang :-(

bad_optional_access::~bad_optional_access() noexcept = default;

const char* bad_optional_access::what() const noexcept {return "bad_optional_access";}

} // namespace ws
