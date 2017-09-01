#pragma once
#include <optional>
#include <vector>
#include "gsl/assert.hpp"

namespace ws {

struct frame
{
    static const char continuation = char{0b0000};
    static const char text = char{0b1000};   // 1
    static const char binary = char{0b0010}; // 2
    static const char close = char{0b1000};  // 8
    static const char ping = char{0b1001};   // 9
    static const char pong = char{0b1010};   // 10

    union alignas(16)
    {
        std::uint16_t bits;
        struct alignas(16)
        {
            std::uint16_t fin : 1;
            std::uint16_t rsv1 : 1;
            std::uint16_t rsv2 : 1;
            std::uint16_t rsv3 : 1;
            std::uint16_t opcode : 4;
            std::uint16_t payload_length : 7;
            std::uint16_t masked : 1;
        } header;
    };

    std::optional<std::uint16_t> extended_payload_length_16;
    std::optional<std::uint64_t> extended_payload_length_64;
    std::optional<std::uint32_t>  masking_key;
    std::vector<char> extension_data;
    std::vector<char> payload_data;
};

} // namespace ws

namespace std
{

template<typename CharT>
auto& operator << (std::basic_ostream<CharT>& os, const ws::frame& f)
{
    // Expects((f.header.payload_length != std::byte{126} && f.header.payload_length != std::byte{127}) ||
    //         (f.header.payload_length == std::byte{126} && f.extended_payload_length_16.has_value())  ||
    //         (f.header.payload_length == std::byte{127} && f.extended_payload_length_64.has_value())  );
    // Expects((f.header.masked == std::byte{0b0} && !f.masking_key.has_value()) ||
    //         (f.header.masked == std::byte{0b1} && f.masking_key.has_value())  );

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
    is.read(reinterpret_cast<CharT*>(&f.bits), 2);
    if(f.header.payload_length == 126)
    {
        is.read(reinterpret_cast<CharT*>(&length), 2);
        f.extended_payload_length_16 = length;
    }
    else if(f.header.payload_length == 127)
    {
        is.read(reinterpret_cast<CharT*>(&length), 8);
        f.extended_payload_length_64 = length;
    }
    else
    {
        length = f.header.payload_length;
    }
    if(f.header.masked)
    {
        auto key = std::uint32_t{};
        is.read(reinterpret_cast<CharT*>(&key), 4);
        f.masking_key = key;
    }
    while (length--)
        f.payload_data.push_back(is.get());

    // Ensures((f.header.payload_length != std::byte{126} && f.header.payload_length != std::byte{127}) ||
    //         (f.header.payload_length == std::byte{126} && f.extended_payload_length_16.has_value())  ||
    //         (f.header.payload_length == std::byte{127} && f.extended_payload_length_64.has_value()) );
    // Ensures((f.header.masked == std::byte{0b0} && !f.masking_key.has_value()) ||
    //         (f.header.masked == std::byte{0b1} && f.masking_key.has_value())  );

    return is;
}

// Hack to get optional working with clang :-(

bad_optional_access::~bad_optional_access() noexcept = default;

const char* bad_optional_access::what() const noexcept {return "bad_optional_access";}

} // namespace ws
