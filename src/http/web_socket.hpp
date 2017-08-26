#pragma once

namespace ws
{
    struct frame
    {
        std::byte fin : 1;
        std::byte rsv1 : 1;
        std::byte rsv2 : 1;
        std::byte rsv3 : 1;
        std::byte opcode : 4;
        std::uint32_t masked;
        std::byte payload_length : 7;
        std::optional<std::uint16_t> extended_payload_length_16;
        std::optional<std::uint64_t> extended_payload_length_64;
        std::optional<std::byte> masking_key : 4;
        std::vector<std::byte> extension_data
        std::vector<std::byte> payload_data;
    };

} // namespace ws
