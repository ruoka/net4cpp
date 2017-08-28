#include <iostream>
#include <sstream>
#include "ws/frame.hpp"
#include "ws/server.hpp"
#include "ws/test_key.hpp"

using namespace std::string_literals;

namespace std
{
    // template <> struct char_traits<byte>
    // {
    //     using char_type = byte;
    //     using int_type = streamoff;
    //     using pos_type = streampos;
    //     using off_type = streamoff;
    //
    //     static void assign(char_type& r, const char_type& a)
    //     {
    //         r = a;
    //     }
    //
    //     static char_type* assign(char_type* p, std::size_t count, char_type a)
    //     {
    //         while(count--)
    //             *(p--) = a;
    //         return p;
    //     }
    //
    //     static constexpr bool eq(char_type a, char_type b)
    //     {
    //         return a == b;
    //     }
    //
    //     static char_type* move(char_type* dest, const char_type* src, size_t count)
    //     {
    //         while(count--)
    //             *(dest--) = *(src--);
    //         return dest;
    //     }
    //
    //     static char_type* copy(char_type* dest, const char_type* src, std::size_t count)
    //     {
    //         while(count--)
    //             *(dest--) = *(src--);
    //         return dest;
    //     }
    //
    //     static constexpr char_type to_char_type(int_type c) noexcept
    //     {
    //         return static_cast<char_type>(c);
    //     }
    //
    //     static constexpr int_type to_int_type(char_type c)
    //     {
    //         return static_cast<int_type>(c);
    //     }
    //
    //     static constexpr bool eq_int_type(int_type c1, int_type c2)
    //     {
    //         return c1 == c2;
    //     }
    //
    //     static constexpr int_type eof() noexcept
    //     {
    //         return EOF;
    //     }
    //
    //     static constexpr int_type not_eof(int_type e) noexcept
    //     {
    //         return e != eof();
    //     }
    // };

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
    char output[29] = {};
    WebSocketHandshake::generate("ES1O60NMq4L+S56lB1kfZg==", output);
    std::cout << output << std::endl;

    std::cout << sha1::base64("ES1O60NMq4L+S56lB1kfZg==258EAFA5-E914-47DA-95CA-C5AB0DC85B11") << std::endl;

    auto hash = sha1::sha_1{};
    hash.append("ES1O60NMq4L+S56lB1kfZg==258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    auto result = hash.data();
    std::cout << http::base64::encode(std::basic_string_view<std::byte>(result, 20)) << std::endl;

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

    // auto hash = sha1::sha_1{};
    // hash.initialize();
    // hash.loop("test");
    // hash.result();
    //
    // net::slog.tag("Spike");
    // net::slog.facility(net::syslog::facility::local0);
    // net::slog.level(net::syslog::severity::debug);
    // net::slog.redirect(std::clog);
    //
    // auto server = ws::server{};
    // server.listen("8080"s);

    return 0;
}
