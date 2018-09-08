#pragma once
#include <limits>

namespace cryptic {

using namespace std;

template<typename Integer>
constexpr byte narrow(Integer number)
{
    static_assert(is_integral_v<Integer>);
    static_assert(numeric_limits<byte>::digits < numeric_limits<Integer>::digits);
    return static_cast<byte>(number bitand 0b11111111);
}

template<size_t Rotation, typename Unsigned>
constexpr Unsigned rightrotate(Unsigned number)
{
    static_assert(is_unsigned_v<Unsigned>);
    constexpr auto bits = numeric_limits<Unsigned>::digits;
    static_assert(Rotation <= bits);
    return (number >> Rotation) bitor (number << (bits - Rotation));
}

template<size_t Rotation, typename Unsigned>
constexpr Unsigned leftrotate(Unsigned number)
{
    static_assert(is_unsigned_v<Unsigned>);
    constexpr auto bits = numeric_limits<Unsigned>::digits;
    static_assert(Rotation <= bits);
    return (number << Rotation) bitor (number >> (bits - Rotation));
}

} // namespace cryptic
