#include <iostream>
#include <sstream>
#include <fstream>
#include "ws/frame.hpp"
#include "ws/server.hpp"
#include "ws/test_key.hpp"
#include "ws/sha1.hpp"
#include "cryptic/sha1.hpp"

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
    std::cout << sha1::base64("") << std::endl;

    auto hash = cryptic::sha1{};
    hash.update(cryptic::span<const cryptic::byte>{nullptr});
    std::cout << hash.base64() << std::endl;

    std::cout << sha1::base64("XX") << std::endl;

    std::array<std::byte,2> data = {std::byte{'X'},std::byte{'X'}};
    hash = cryptic::sha1{};
    hash.update(data);
    std::cout << hash.base64() << std::endl;

    auto test1 = "The quick brown fox jumps over the lazy dog"s;

    std::cout << sha1::base64(test1) << std::endl;

    hash = cryptic::sha1{};
    hash.update(test1);
    std::cout << hash.base64() << std::endl;

    auto file = std::ifstream{"./src/spike.cpp"};
    auto test2 = ""s;
    std::getline(file,test2,(char)std::char_traits<char>::eof());

    std::clog << test2 << std::endl;

    std::cout << sha1::base64(test2) << std::endl;

    hash = cryptic::sha1{test2};
    std::cout << hash.base64() << std::endl;

    auto test3 = "omQGMC65WBEzzZAX7H8l+g==258EAFA5-E914-47DA-95CA-C5AB0DC85B11"s;

    std::cout << sha1::base64(test3) << std::endl;

    hash = cryptic::sha1{test3};
    std::cout << hash.base64() << std::endl;

    auto test4 = "omQGMC65WBEzzZAX7H8l+g==258EAFA5-E914-47DA-95CA-C5AB0DC85B11_XXXXXX"s;

    std::cout << sha1::base64(test4) << std::endl;

    hash = cryptic::sha1{test4};
    std::cout << hash.base64() << std::endl;

    // auto f1 = ws::frame{};
    // f1.header.fin = std::byte{0b1};
    // f1.header.rsv1 = std::byte{0b0};
    // f1.header.rsv2 = std::byte{0b0};
    // f1.header.rsv3 = std::byte{0b0};
    // f1.header.opcode = ws::frame::pong;
    // f1.header.masked = std::byte{0b1};
    // f1.header.payload_length = std::byte{0x4};
    // f1.masking_key = std::uint32_t{0x13};
    // f1.payload_data.push_back(std::byte{'A'});
    // f1.payload_data.push_back(std::byte{'B'});
    // f1.payload_data.push_back(std::byte{'C'});
    // f1.payload_data.push_back(std::byte{'D'});

    // std::clog << std::to_integer<int>(f1.header.fin) << std::endl;
    // std::clog << std::to_integer<int>(f1.header.masked) << std::endl;
    // std::clog << std::to_integer<int>(f1.header.opcode) << std::endl;
    // std::clog << std::to_integer<int>(f1.header.payload_length) << std::endl;

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
