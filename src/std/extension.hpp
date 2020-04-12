#pragma once

#include <chrono>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <string_view>
#include <algorithm>
#include <cctype>

namespace ext {

using days = std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type>;

using years = std::chrono::duration<int, std::ratio_multiply<days::period, std::ratio<365>>::type>;

using months = std::chrono::duration<int, std::ratio_divide<years::period, std::ratio<12>>::type>;

static const std::string number2month[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const std::string number2weekday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr auto& to_string(const months& m) noexcept
{
    const auto n = m.count();
    return ext::number2month[n];
}

constexpr auto& to_string(const days& d) noexcept
{
    const auto z = d.count();
    const auto n = ( z >= -4 ? (z+4) % 7 : (z+5) % 7 + 6);
    return ext::number2weekday[n];
}

// http://howardhinnant.github.io/date_algorithms.html#days_from_civil

constexpr auto to_time_point(const years& ys, const months& ms, const days& ds)
{
    using namespace std::chrono;
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

constexpr auto to_years_days_months(const days& ds)
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
constexpr auto to_years_days_months_hours_minutes_seconds(const std::chrono::time_point<T>& point) noexcept
{
    using namespace std;
    using namespace chrono;
    auto tmp = point;
    const auto ds = duration_cast<days>(tmp.time_since_epoch());
    tmp -= ds;
    const auto hs = duration_cast<hours>(tmp.time_since_epoch());
    tmp -= hs;
    const auto ms = duration_cast<minutes>(tmp.time_since_epoch());
    tmp -= ms;
    const auto ss = duration_cast<seconds>(tmp.time_since_epoch());
    return tuple_cat( to_years_days_months(ds), make_tuple(hs, ms, ss) );
}

template<typename T>
constexpr auto to_years_days_months_hours_minutes_seconds_milliseconds(const std::chrono::time_point<T>& point) noexcept
{
    using namespace std;
    using namespace chrono;
    auto tmp = point;
    const auto ds = duration_cast<days>(tmp.time_since_epoch());
    tmp -= ds;
    const auto hs = duration_cast<hours>(tmp.time_since_epoch());
    tmp -= hs;
    const auto ms = duration_cast<minutes>(tmp.time_since_epoch());
    tmp -= ms;
    const auto ss = duration_cast<seconds>(tmp.time_since_epoch());
    tmp -= ss;
    const auto fs = duration_cast<milliseconds>(tmp.time_since_epoch());
    return tuple_cat( to_years_days_months(ds), make_tuple(hs, ms, ss, fs) );
}

template<typename T>
constexpr auto& to_day_of_week(const std::chrono::time_point<T>& point) noexcept
{
    using namespace std;
    using namespace chrono;
    const auto d = duration_cast<days>(point.time_since_epoch());
    return to_string(d);
}

template<typename T>
auto to_rfc1123(const std::chrono::time_point<T>& point) noexcept
{
    using namespace std;
    using namespace chrono;
    // Sun, 06 Nov 1994 08:49:37 GMT
    const auto date = to_years_days_months_hours_minutes_seconds(point);
    auto os = ostringstream{};
    os << to_day_of_week(point) << ',' << ' '
       << setw(2) << setfill('0') << get<days>(date)       << ' '
       << to_string(get<months>(date))                     << ' '
       << setw(4) << setfill('0') << get<years>(date)      << ' '
       << setw(2) << setfill('0') << get<hours>(date)      << ':'
       << setw(2) << setfill('0') << get<minutes>(date)    << ':'
       << setw(2) << setfill('0') << get<seconds>(date)    << ' '
       << "GMT";
    return os.str();
}

template<typename T>
auto to_iso8601(const std::chrono::time_point<T>& point) noexcept
{
    using namespace std;
    using namespace std::chrono;
    // YYYY-MM-DDThh:mm:ss.fffZ
    const auto date = to_years_days_months_hours_minutes_seconds_milliseconds(point);
    auto os = ostringstream{};
    os << setw(4) << setfill('0') << get<years>(date)        << '-'
       << setw(2) << setfill('0') << get<months>(date)       << '-'
       << setw(2) << setfill('0') << get<days>(date)         << 'T'
       << setw(2) << setfill('0') << get<hours>(date)        << ':'
       << setw(2) << setfill('0') << get<minutes>(date)      << ':'
       << setw(2) << setfill('0') << get<seconds>(date)      << '.'
       << setw(3) << setfill('0') << get<milliseconds>(date) << 'Z';
    return os.str();
}

inline auto to_time_point(const std::string& str)
{
    // YYYY-MM-DDThh:mm:ss.fffZ
    using namespace std::chrono;
    const auto YY = years{stoi(str.substr(0,4))};
    const auto MM = months{stoi(str.substr(5,2))};
    const auto DD = days{stoi(str.substr(8,2))};
    const auto hh = hours{stoi(str.substr(11,2))};
    const auto mm = minutes{stoi(str.substr(14,2))};
    const auto ss = seconds {stoi(str.substr(17,2))};
    const auto ff = milliseconds{stoi(str.substr(20,3))};
    auto point = to_time_point(YY,MM,DD);
    point += hh; point += mm; point += ss; point += ff;
    return point;
}

inline auto to_boolean(const std::string& str)
{
    using namespace std::string_literals;
    if(str == "true"s  || str == "1"s) return true;
    if(str == "false"s || str == "0"s) return false;
    throw std::invalid_argument{"No conversion to bool could be done for '"s + str + "'"s};
}

inline void to_upper(std::string& str) noexcept
{
    for(auto& c : str) c = std::toupper(c);
}

inline std::string& trim_right(std::string& str, const std::string& delimiters = " \f\n\r\t\v")
{
    return str.erase(str.find_last_not_of(delimiters) + 1);
}

inline std::string& trim_left(std::string& str, const std::string& delimiters = " \f\n\r\t\v")
{
      return str.erase(0, str.find_first_not_of(delimiters));
}

inline std::string& trim(std::string& str, const std::string& delimiters = " \f\n\r\t\v")
{
      return trim_left(trim_right(str, delimiters ), delimiters);
}

template<typename T>
bool isnumeric(const T& str)
{
    return str.find_first_not_of("0123456789") == str.npos;
}

static const std::string bool2string[] = {"false", "true"};

static const std::string null2string = {"null"};


template<typename T>
std::string to_string(const std::chrono::time_point<T>& point) noexcept
{
    return ext::to_iso8601(point);
}

constexpr const std::string& to_string(bool b) noexcept
{
    return ext::bool2string[b];
}

constexpr const std::string& to_string(std::nullptr_t) noexcept
{
    return ext::null2string;
}

constexpr const std::string& to_string(const std::string& str) noexcept
{
    return str;
}

inline std::string to_string(std::string_view sv) noexcept
{
    return std::string{sv};
}

inline auto stoi(std::string_view sv, std::size_t* pos = nullptr, int base = 10)
{
    char* end;
    auto i = std::strtol(sv.data(), &end, base);
    if(pos) *pos = std::distance<const char*>(sv.data(), end);
    return static_cast<std::int32_t>(i);
}

inline auto stol(std::string_view sv, std::size_t* pos = nullptr, int base = 10)
{
    char* end;
    auto i = std::strtol(sv.data(), &end, base);
    if(pos) *pos = std::distance<const char*>(sv.data(), end);
    return i;
}

inline auto stoll(std::string_view sv, std::size_t* pos = nullptr, int base = 10)
{
    char* end;
    auto ll = std::strtoll(sv.data(), &end, base);
    if(pos) *pos = std::distance<const char*>(sv.data(), end);
    return ll;
}

inline std::string operator+(std::string& str, std::string_view sv)
{
	return str.append(sv.data(), sv.size());
}

} // namespace ext

namespace std {

template<typename T, typename R>
auto& operator << (std::ostream& os, const std::chrono::duration<T,R>& d) noexcept
{
    os << d.count();
    return os;
}

} // namespace std
