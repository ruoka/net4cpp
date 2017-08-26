#include <iostream>
#include <sstream>
#include "http/web_socket.hpp"

int main()
{
    auto f1 = ws::frame{};
    f1.header.fin = std::byte{0b1};
    f1.header.rsv1 = std::byte{0b0};
    f1.header.rsv2 = std::byte{0b0};
    f1.header.rsv3 = std::byte{0b0};
    f1.header.opcode = ws::frame::pong;
    f1.header.masked = std::byte{0b1};
    f1.header.payload_length = std::byte{0x4};
    f1.masking_key = std::uint32_t{0x13};
    f1.payload_data.push_back(std::byte{'A'});
    f1.payload_data.push_back(std::byte{'B'});
    f1.payload_data.push_back(std::byte{'C'});
    f1.payload_data.push_back(std::byte{'D'});

    std::clog << std::to_integer<int>(f1.header.fin) << std::endl;
    std::clog << std::to_integer<int>(f1.header.masked) << std::endl;
    std::clog << std::to_integer<int>(f1.header.opcode) << std::endl;
    std::clog << std::to_integer<int>(f1.header.payload_length) << std::endl;

    std::clog << f1 << std::endl;

    std::clog << "payload_length = " << std::to_integer<std::size_t>(f1.header.payload_length) << std::endl;

    auto ss = std::stringstream{};

    ss << f1;

    std::clog << ss.str() << std::endl;

    auto f2 = ws::frame{};

    ss >> f2;

    std::clog << "fin = " << std::to_integer<int>(f2.header.fin) << std::endl;
    std::clog << "opcode = " << std::to_integer<int>(f2.header.opcode) << std::endl;
    std::clog << "payload_length = " << std::to_integer<std::size_t>(f2.header.payload_length) << std::endl;

    return 0;
}
