#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include <charconv>

namespace ext
{

inline auto& to_string(const std::chrono::month& m) noexcept
{
    static const std::string number2month[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const auto n = static_cast<unsigned>(m);
    return number2month[n];
}

inline auto& to_string(const std::chrono::weekday& wd) noexcept
{
    static const std::string number2weekday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
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
    os << ext::to_string(weekday)                                     << ", "
       << std::setw(2) << std::setfill('0') << (unsigned)date.day()   << ' '
       << ext::to_string(date.month())                                << ' '
       << std::setw(4) << std::setfill('0') << (int)date.year()       << ' '
       << std::setw(2) << std::setfill('0') << time.hours().count()   << ':'
       << std::setw(2) << std::setfill('0') << time.minutes().count() << ':'
       << std::setw(2) << std::setfill('0') << time.seconds().count() << ' '
       << "GMT";
    return os.str();
}

inline auto to_time_point(const std::string_view sv)
{
    // YYYY-MM-DDTHH:MM:SS.sssZ
    assert(sv.size() > 9);
    auto YYYY = 0; auto MM = 0u, DD = 0u, hh = 0u, mm = 0u, ss = 0u, fff = 0u;
    auto res = std::from_chars_result{sv.data(),std::errc()};
    res = std::from_chars(res.ptr,res.ptr+4,YYYY);
    res = std::from_chars(++res.ptr,res.ptr+2,MM);
    res = std::from_chars(++res.ptr,res.ptr+2,DD);
    if(sv.length() == 24)
    {
        res = std::from_chars(++res.ptr,res.ptr+2,hh);
        res = std::from_chars(++res.ptr,res.ptr+2,mm);
        res = std::from_chars(++res.ptr,res.ptr+2,ss);
        res = std::from_chars(++res.ptr,res.ptr+3,fff);
    }
    using namespace std::chrono;
    return sys_days{year{YYYY}/month{MM}/day{DD}} + hours{hh} + minutes{mm} + seconds{ss} + milliseconds{fff};
}

inline void to_upper(std::string& str) noexcept
{
    for(auto& c : str) c = std::toupper(c);
}

inline void to_lower(std::string& str) noexcept
{
    for(auto& c : str) c = std::tolower(c);
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

} // namespace ext
