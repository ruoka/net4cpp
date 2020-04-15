#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include <string_view>

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

namespace chrono
{
template<class Duration> class hh_mm_ss
{
public:

    static constexpr unsigned fractional_width = 6; // FIXME

    using precision = duration<common_type_t<typename Duration::rep, std::chrono::seconds::rep>, ratio<1,1000000>>; // FIXME

    constexpr hh_mm_ss() noexcept : hh_mm_ss{Duration::zero()} {};

    constexpr explicit hh_mm_ss(Duration d) :
        is_neg{d < Duration{0}},
        h{duration_cast<chrono::hours>(std::chrono::abs(d))},
        m{duration_cast<chrono::minutes>(std::chrono::abs(d) - hours())},
        s{duration_cast<chrono::seconds>(std::chrono::abs(d) - hours() - minutes())},
        ss{duration_cast<precision>(std::chrono::abs(d) - hours() - minutes() - seconds())}
    {}

    constexpr bool is_negative() const noexcept {return is_neg;};

    constexpr chrono::hours hours() const noexcept {return h;};

    constexpr chrono::minutes minutes() const noexcept {return m;};

    constexpr chrono::seconds seconds() const noexcept {return s;};

    constexpr precision subseconds() const noexcept {return ss;};

    constexpr precision to_duration() const noexcept
    {
        auto dur = h + m + s + ss;
        return is_neg ? -dur : dur;
    }

    constexpr explicit operator precision() const noexcept {return to_duration();};

private:
    bool is_neg;
    std::chrono::hours h;
    std::chrono::minutes m;
    std::chrono::seconds s;
    precision ss;
};

} // namespace chrono
} // namespace std

namespace ext
{

inline auto stoi(std::string_view sv, std::size_t* pos = nullptr, int base = 10)
{
    char* end;
    auto i = std::strtol(sv.data(), &end, base);
    if(pos) *pos = std::distance<const char*>(sv.data(), end);
    return static_cast<std::int32_t>(i);
}

inline auto stou(std::string_view sv, std::size_t* pos = nullptr, int base = 10)
{
    char* end;
    auto i = std::strtol(sv.data(), &end, base);
    if(pos) *pos = std::distance<const char*>(sv.data(), end);
    return static_cast<std::uint32_t>(i);
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

inline std::string operator+(std::string str, std::string_view sv)
{
	return str.append(sv.data(), sv.size());
}

static const std::string g_number2month[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

constexpr auto& to_string(const std::chrono::month& m) noexcept
{
    const auto n = static_cast<unsigned>(m);
    return g_number2month[n];
}

static const std::string g_number2weekday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr auto& to_string(const std::chrono::weekday& wd) noexcept
{
    const auto n = static_cast<unsigned>(wd);
    return g_number2weekday[n];
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

inline auto to_time_point(const std::string_view sv)
{
    // YYYY-MM-DDThh:mm:ss.fffZ
    using namespace std::chrono;
    const auto YY = year{stoi(sv.substr(0,4))};
    const auto MM = month{stou(sv.substr(5,2))};
    const auto DD = day{stou(sv.substr(8,2))};
    const auto hh = hours{stoi(sv.substr(11,2))};
    const auto mm = minutes{stoi(sv.substr(14,2))};
    const auto ss = seconds {stoi(sv.substr(17,2))};
    const auto ff = milliseconds{stoi(sv.substr(20,3))};
    auto point = time_point_cast<milliseconds>(sys_days{YY/MM/DD});
    point += hh; point += mm; point += ss; point += ff;
    return point;
}

template<typename T>
std::string to_string(const std::chrono::time_point<T>& point) noexcept
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

static const std::string g_bool2string[] = {"false", "true"};

constexpr const std::string& to_string(bool b) noexcept
{
    return g_bool2string[b];
}

static const std::string g_null2string = {"null"};

constexpr const std::string& to_string(std::nullptr_t) noexcept
{
    return g_null2string;
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
