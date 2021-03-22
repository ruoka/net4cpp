#pragma once

#include <chrono>
#include <array>
#include <sstream>
#include <iomanip>
#include <string_view>
using namespace std::literals;

namespace std
{

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const std::chrono::year& y) noexcept
{
    os << static_cast<int>(y);
    return os;
}

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const std::chrono::month& m) noexcept
{
    os << static_cast<unsigned>(m);
    return os;
}

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os, const std::chrono::day& d) noexcept
{
    os << static_cast<unsigned>(d);
    return os;
}

template<typename T, typename R>
auto& operator << (std::ostream& os, const std::chrono::duration<T,R>& d) noexcept
{
    os << d.count();
    return os;
}

} // namespace std

namespace ext
{

inline auto stoi(std::string_view sv)
{
    auto i = 0;
    if(sv.front() != '-')
        for(const auto c : sv)
        {
            i *= 10;
            i += c -'0';
        }
    else
        for(const auto c : sv.substr(1))
        {
            i *= 10;
            i -= c -'0';
        }
    return i;
}

inline auto stol(std::string_view sv)
{
    auto i = 0l;
    if(sv.front() != '-')
        for(const auto c : sv)
        {
            i *= 10;
            i += c -'0';
        }
    else
        for(const auto c : sv.substr(1))
        {
            i *= 10;
            i -= c -'0';
        }
    return i;
}

inline auto stoll(std::string_view sv)
{
    auto i = 0ll;
    if(sv.front() != '-')
        for(const auto c : sv)
        {
            i *= 10;
            i += c -'0';
        }
    else
        for(const auto c : sv.substr(1))
        {
            i *= 10;
            i -= c -'0';
        }
    return i;
}

inline auto stou(std::string_view sv)
{
    auto i = 0u;
    for(const auto c : sv)
    {
        i *= 10;
        i += c -'0';
    }
    return i;
}

inline auto stoul(std::string_view sv)
{
    auto i = 0ul;
    for(const auto c : sv)
    {
        i *= 10;
        i += c -'0';
    }
    return i;
}

inline auto stoull(std::string_view sv)
{
    auto i = 0ull;
    for(const auto c : sv)
    {
        i *= 10;
        i += c -'0';
    }
    return i;
}

inline std::string operator+(std::string str, std::string_view sv)
{
	return str.append(std::string{sv}, sv.size());
}

inline const std::string& to_string(const std::chrono::month& m) noexcept
{
    static const auto number2month = std::array{""s, "Jan"s, "Feb"s, "Mar"s, "Apr"s, "May"s, "Jun"s, "Jul"s, "Aug"s, "Sep"s, "Oct"s, "Nov"s, "Dec"s};
    const auto n = static_cast<unsigned>(m);
    return number2month[n];
}

inline const std::string& to_string(const std::chrono::weekday& wd) noexcept
{
    static const auto number2weekday = std::array{"Sun"s, "Mon"s, "Tue"s, "Wed"s, "Thu"s, "Fri"s, "Sat"s};
    const auto n = wd.c_encoding();
    return number2weekday[n];
}

template<typename T>
auto to_rfc1123(const std::chrono::time_point<T>& current_time) noexcept
{
    const auto midnight = std::chrono::floor<std::chrono::days>(current_time);
    const auto weekday = std::chrono::weekday{midnight};
    const auto date = std::chrono::year_month_day{midnight};
    const auto time = std::chrono::hh_mm_ss{current_time - midnight};
    // Sun, 22 Feb 2112 10:00:00
    auto os = std::ostringstream{};
    os << ext::to_string(weekday)                             << ", "
       << std::setw(2) << std::setfill('0') << date.day()     << ' '
       << ext::to_string(date.month())                        << ' '
       << std::setw(4) << std::setfill('0') << date.year()    << ' '
       << std::setw(2) << std::setfill('0') << time.hours()   << ':'
       << std::setw(2) << std::setfill('0') << time.minutes() << ':'
       << std::setw(2) << std::setfill('0') << time.seconds() << ' '
       << "GMT";
    return os.str();
}

template<typename T>
auto to_iso8601(const std::chrono::time_point<T>& current_time) noexcept
{
    const auto midnight = std::chrono::floor<std::chrono::days>(current_time);
    const auto date = std::chrono::year_month_day{midnight};
    const auto time = std::chrono::hh_mm_ss{current_time - midnight};
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.subseconds());
    // YYYY-MM-DDThh:mm:ss.fffZ
    auto os = std::ostringstream{};
    os << std::setw(4) << std::setfill('0') << date.year()    << '-'
       << std::setw(2) << std::setfill('0') << date.month()   << '-'
       << std::setw(2) << std::setfill('0') << date.day()     << 'T'
       << std::setw(2) << std::setfill('0') << time.hours()   << ':'
       << std::setw(2) << std::setfill('0') << time.minutes() << ':'
       << std::setw(2) << std::setfill('0') << time.seconds() << '.'
       << std::setw(3) << std::setfill('0') << milliseconds   << 'Z';
    return os.str();
}

template<typename T>
auto to_utc(const std::chrono::time_point<T>& current_time) noexcept
{
    const auto midnight = std::chrono::floor<std::chrono::days>(current_time);
    const auto date = std::chrono::year_month_day{midnight};
    const auto time = std::chrono::hh_mm_ss{current_time - midnight};
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.subseconds());
    // YYYYMMDD-HH:MM:SS.sss
    auto os = std::ostringstream{};
    os << std::setw(4) << std::setfill('0') << date.year()
       << std::setw(2) << std::setfill('0') << date.month()
       << std::setw(2) << std::setfill('0') << date.day()     << '-'
       << std::setw(2) << std::setfill('0') << time.hours()   << ':'
       << std::setw(2) << std::setfill('0') << time.minutes() << ':'
       << std::setw(2) << std::setfill('0') << time.seconds() << '.'
       << std::setw(3) << std::setfill('0') << milliseconds;
    return os.str();
}

inline auto to_time_point(const std::string_view sv)
{
    // YYYY-MM-DDThh:mm:ss.fffZ
    using namespace std::chrono;
    const auto YYYY = year{stoi(sv.substr(0,4))};
    const auto MM = month{stou(sv.substr(5,2))};
    const auto DD = day{stou(sv.substr(8,2))};
    const auto hh = hours{stoi(sv.substr(11,2))};
    const auto mm = minutes{stoi(sv.substr(14,2))};
    const auto ss = seconds {stoi(sv.substr(17,2))};
    const auto ff = milliseconds{stoi(sv.substr(20,3))};
    return sys_days{YYYY/MM/DD} + hh + mm + ss + ff + 0us;
}

template<typename T>
auto to_string(const std::chrono::time_point<T>& point) noexcept
{
    return ext::to_iso8601(point);
}

inline auto to_boolean(std::string_view sv)
{
    using namespace std::literals;
    if(sv == "true"  || sv == "1") return true;
    if(sv == "false" || sv == "0") return false;
    throw std::invalid_argument{"No conversion to bool could be done for '"s + sv + "'"s};
}

inline const std::string& to_string(bool b) noexcept
{
    static const auto bool2string = std::array{"false"s, "true"s};
    return bool2string[b];
}

inline std::string to_string(std::nullptr_t) noexcept
{
    return "null";
}

inline std::string to_string(std::string_view sv) noexcept
{
    return std::string{sv};
}

constexpr const std::string& to_string(const std::string& str) noexcept
{
    return str;
}

inline void to_upper(std::string& str) noexcept
{
    for(auto& c : str) c = std::toupper(c);
}

inline std::string& trim_right(std::string& str, const char* ws = " \f\n\r\t\v")
{
    return str.erase(str.find_last_not_of(ws) + 1);
}

inline std::string& trim_left(std::string& str, const char* ws = " \f\n\r\t\v")
{
      return str.erase(0, str.find_first_not_of(ws));
}

inline std::string& trim(std::string& str, const char* ws = " \f\n\r\t\v")
{
      return trim_left(trim_right(str, ws ), ws);
}

template<typename T>
bool isnumeric(const T& str)
{
    return str.find_first_not_of("0123456789") == str.npos;
}

} // namespace ext
