#pragma once

#include <chrono>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>

namespace std {
namespace chrono {

using days = duration<int, ratio_multiply<hours::period, ratio<24>>::type>;

using years = duration<int, ratio_multiply<days::period, ratio<365>>::type>;

using months = duration<int, ratio_divide<years::period, ratio<12>>::type>;

// http://howardhinnant.github.io/date_algorithms.html#days_from_civil

constexpr auto convert(const years& ys, const months& ms, const days& ds)
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
          auto y = static_cast<int>(ys.count());
    const auto m = static_cast<unsigned>(ms.count());
    const auto d = static_cast<unsigned>(ds.count());
    y -= m <= 2;
    const auto era = (y >= 0 ? y : y-399) / 400;
    const auto yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
    const auto doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;  // [0, 365]
    const auto doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
    return system_clock::time_point{days{era * 146097 + doe - 719468}};
}

// http://howardhinnant.github.io/date_algorithms.html#civil_from_days

constexpr auto convert(const days& ds)
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    const auto z = ds.count() + 719468;
    const auto era = (z >= 0 ? z : z - 146096) / 146097;
    const auto doe = static_cast<unsigned>(z - era * 146097);          // [0, 146096]
    const auto yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;  // [0, 399]
    const auto y = static_cast<int>(yoe) + era * 400;
    const auto doy = doe - (365*yoe + yoe/4 - yoe/100);                // [0, 365]
    const auto mp = (5*doy + 2)/153;                                   // [0, 11]
    const auto d = doy - (153*mp+2)/5 + 1;                             // [1, 31]
    const auto m = mp + (mp < 10 ? 3 : -9u);                           // [1, 12]
    return make_tuple( years{y + (m <= 2)}, months{m}, days{d} );
}

template<typename T>
constexpr auto convert(const time_point<T>& tp) noexcept
{
    auto tmp = tp;
    const auto ds = duration_cast<days>(tmp.time_since_epoch());
    tmp -= ds;
    const auto hs = duration_cast<hours>(tmp.time_since_epoch());
    tmp -= hs;
    const auto ms = duration_cast<minutes>(tmp.time_since_epoch());
    tmp -= ms;
    const auto ss = duration_cast<seconds>(tmp.time_since_epoch());
    tmp -= ss;
    const auto fs = duration_cast<milliseconds>(tmp.time_since_epoch());
    return tuple_cat( convert(ds), make_tuple(hs, ms, ss, fs) );
}

template<typename T, typename R>
inline auto& operator << (std::ostream& os, const duration<T,R>& d) noexcept
{
    os << d.count();
    return os;
}

} // namespace chrono

inline auto& to_string(const chrono::months& m)
{
    static const std::string name[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return name[m.count()];
}

template<typename T>
inline auto to_string(const chrono::time_point<T>& tp) noexcept
{
    // YYYY-MM-DDThh:mm:ss.fffZ
    using namespace chrono;
    const auto timestamp = convert(tp);
    auto os = ostringstream{};
    os << setw(4) << setfill('0') << get<years>(timestamp)        << '-'
       << setw(2) << setfill('0') << get<months>(timestamp)       << '-'
       << setw(2) << setfill('0') << get<days>(timestamp)         << 'T'
       << setw(2) << setfill('0') << get<hours>(timestamp)        << ':'
       << setw(2) << setfill('0') << get<minutes>(timestamp)      << ':'
       << setw(2) << setfill('0') << get<seconds>(timestamp)      << '.'
       << setw(3) << setfill('0') << get<milliseconds>(timestamp) << 'Z';
    return os.str();
}

inline auto stotp(const string& str)
{
    // YYYY-MM-DDThh:mm:ss.fffZ
    using namespace chrono;
    const auto YY = years{stoi(str.substr(0,4))};
    const auto MM = months{stoi(str.substr(5,2))};
    const auto DD = days{stoi(str.substr(8,2))};
    const auto hh = hours{stoi(str.substr(11,2))};
    const auto mm = minutes{stoi(str.substr(14,2))};
    const auto ss = seconds {stoi(str.substr(17,2))};
    const auto ff = milliseconds{stoi(str.substr(20,3))};
    auto tp = convert(YY,MM,DD);
    tp += hh; tp += mm; tp += ss; tp += ff;
    return tp;
}

inline auto to_string(bool b) noexcept
{
    auto ss = std::ostringstream{};
    ss << std::boolalpha << b;
    return ss.str();
}

inline auto stob(const string& str) noexcept
{
    auto b = bool{};
    auto ss = std::stringstream{};
    ss << str;
    ss >> boolalpha >> b;
    if(!ss) throw std::invalid_argument{"No conversion to bool could be done for '"s + str + "'"s};
    return b;
}

inline auto to_string(const std::nullptr_t&) noexcept
{
    return "null"s;
}

inline auto to_string(const string& str) noexcept
{
    return str;
}

template <typename T, int N> struct is_array<std::array<T,N>> : std::true_type {};

template <typename T> struct is_array<std::vector<T>> : std::true_type {};

} // namespace std
