#include <iostream>
#include <sstream>
#include <fstream>
#include "ws/frame.hpp"
#include "ws/server.hpp"

using namespace std::string_literals;

namespace std
{
    template <> struct ctype<byte>  : std::ctype<char>
    {
        bool is(mask m, byte c) const
        {
            return false;
        }
    };

} // namespace std

int main()
{
    auto f1 = ws::frame{};
    f1.header.fin = 0b1;
    f1.header.rsv1 = 0b0;
    f1.header.rsv2 = 0b0;
    f1.header.rsv3 = 0b0;
    f1.header.opcode = ws::pong;
    f1.header.masked = 0b1;
    f1.header.payload_length = 0x4;
    f1.masking_key = 0x13;
    // f1.payload_data.push_back(std::byte{'A'});
    // f1.payload_data.push_back(std::byte{'B'});
    // f1.payload_data.push_back(std::byte{'C'});
    // f1.payload_data.push_back(std::byte{'D'});

    std::clog << std::bitset<1>{f1.header.fin}            << std::endl;
    std::clog << std::bitset<1>{f1.header.masked}         << std::endl;
    std::clog << std::bitset<4>{f1.header.opcode}         << std::endl;
    std::clog << std::bitset<7>{f1.header.payload_length} << std::endl;

    // std::clog << f1 << std::endl;
    //
    // std::clog << "payload_length = " << std::to_integer<std::size_t>(f1.header.payload_length) << std::endl;
    //
    // auto ss = std::basic_stringstream<std::byte>{};
    //
    // ss << f1;
    //
    // // std::clog << ss.str() << std::endl;
    //
    // auto f2 = ws::frame{};
    //
    // ss >> f2;
    //
    // std::clog << "fin = " << std::to_integer<int>(f2.header.fin) << std::endl;
    // std::clog << "opcode = " << std::to_integer<int>(f2.header.opcode) << std::endl;
    // std::clog << "payload_length = " << std::to_integer<std::size_t>(f2.header.payload_length) << std::endl;

    net::slog.tag("Spike");
    net::slog.facility(net::syslog::facility::local0);
    net::slog.level(net::syslog::severity::debug);
    net::slog.redirect(std::clog);

    auto server = ws::server{};
    server.listen("8080"s);

    return 0;
}
