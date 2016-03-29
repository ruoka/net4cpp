#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>

namespace std
{

namespace chrono
{

using days = duration<int, ratio_multiply<hours::period, ratio<24>>::type>;

using years = duration<int, ratio_multiply<days::period, ratio<365>>::type>;

using months = duration<int, ratio_divide<years::period, ratio<12>>::type>;

constexpr auto convert(const days& ds) noexcept
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
    return make_tuple(years{y + (m <= 2)}, months{m}, days{d});
}

template<typename T>
inline auto convert(const time_point<T>& tp) noexcept
{
    auto tmp = tp;
    const auto dd = duration_cast<days>(tmp.time_since_epoch());
    years YY;
    months MM;
    days DD;
    tie(YY,MM,DD) = convert(dd);
    tmp -= dd;
    const auto hh = duration_cast<hours>(tmp.time_since_epoch());
    tmp -= hh;
    const auto mm = duration_cast<minutes>(tmp.time_since_epoch());
    tmp -= mm;
    const auto ss = duration_cast<seconds>(tmp.time_since_epoch());
    tmp -= ss;
    const auto ff = duration_cast<milliseconds>(tp.time_since_epoch());
    return make_tuple(YY,MM,DD,hh,mm,ss,ff);
}

inline auto& operator << (std::ostream& os, const months& m) noexcept
{
    const char* name[] = {"XXX", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    os << name[m.count()];
    return os;
}

} // namespace chrono

template<typename T>
inline auto to_string(const chrono::time_point<T>& tp) noexcept
{
    using namespace chrono;
    years YY;
    months MM;
    days DD;
    hours hh;
    minutes mm;
    seconds ss;
    milliseconds ff;
    tie(YY,MM,DD,hh,mm,ss,ff) = convert(tp);

    ostringstream os;
    os << setw(4) << setfill('0') << YY.count() << '-'
       << setw(2) << setfill('0') << MM.count() << '-'
       << setw(2) << setfill('0') << DD.count() << 'T'
       << setw(2) << setfill('0') << hh.count() << ':'
       << setw(2) << setfill('0') << mm.count() << ':'
       << setw(2) << setfill('0') << ss.count() << '.'
       << setw(3) << setfill('0') << ff.count() << 'Z';

    return os.str();
}

inline auto to_string(bool b) noexcept
{
    std::stringstream ss;
    ss << std::boolalpha << b;
    return ss.str();
}

inline auto to_string(const std::nullptr_t&) noexcept
{
    return "null"s;
}

inline auto to_string(const string& str) noexcept
{
    return str;
}

inline auto stob(const string& str)
{
    bool b;
    stringstream ss;
    ss << str;
    ss >> boolalpha >> b;
    if(!ss) throw std::invalid_argument{"No conversion to bool could be done for '"s + str + "'"s};
    return b;
}

template <typename T, int N> struct is_array<std::array<T,N>> : std::true_type {};

template <typename T> struct is_array<std::vector<T>> : std::true_type {};

} // namespace std
