#pragma once

#include <mutex>
#include <iomanip>
#include <array>
#include <type_traits>
#include "net/endpointstream.hpp"
#include "std/extension.hpp"

namespace net {
namespace syslog {

    enum class facility : int
    {
        user   = 1,
        local0 = 16,
        local1 = 17,
        local2 = 18,
        local3 = 19,
        local4 = 20,
        local5 = 21,
        local6 = 22,
        local7 = 23
    };

    enum class severity : int
    {
         emergency = 0,
         alert     = 1,
         critical  = 2,
         error     = 3,
         warning   = 4,
         notice    = 5,
         info      = 6,
         debug     = 7
    };

    std::string gethostname();

    int getpid();

} // namespace syslog

class syslogstream : public oendpointstream
{
public:

    syslogstream(oendpointstream&& s)
    try :
        oendpointstream{std::move(s)},
        m_facility{syslog::facility::user},
        m_severity{syslog::severity::debug},
        m_level{syslog::severity::debug},
        m_tag{"syslogstream"},
        m_mutex{}
    {}
    catch(...)
    {
        std::cerr << "failed to create datagram socket for syslog" << std::endl;
    }

    void redirect(std::ostream& os)
    {
        rdbuf(os.rdbuf());
    }

    void facility(syslog::facility f)
    {
        m_facility = f;
    }

    template<typename T>
    void facility(T f)
    {
        m_facility = static_cast<syslog::facility>(f);
    }

    void level(syslog::severity s)
    {
        m_level = s;
    }

    template<typename T>
    void level(T s)
    {
        m_level = static_cast<syslog::severity>(s);
    }

    void tag(std::string_view tag)
    {
        m_tag = tag;
    }

    void severity(syslog::severity s)
    {
        m_mutex.lock();
        m_severity = s;
    }

    void header()
    {
        using namespace std;
        static const auto hostname = syslog::gethostname();
        static const auto pid = syslog::getpid();

        if(m_level >= m_severity)
        {
            const auto current_time = std::chrono::system_clock::now();
            const auto midnight = std::chrono::floor<std::chrono::days>(current_time);
            const auto date = std::chrono::year_month_day{midnight};
            const auto time = std::chrono::hh_mm_ss{current_time - midnight};
            const auto formatting = flags();
            // <PRI> Feb 22 21:12 localhost syslog[2112]:
            static_cast<oendpointstream&>(*this)
                    << std::resetiosflags(formatting)
                    << '<' << priority(m_facility, m_severity)                   << '>'  // <PRI>
                    << std::setw(3) << ext::to_string(date.month())              << ' '  // TIMESTAMP
                    << std::setw(2) << std::setfill(' ') << date.day()           << ' '
                    << std::setw(2) << std::setfill('0') << time.hours()         << ':'
                    << std::setw(2) << std::setfill('0') << time.minutes()       << ':'
                    << std::setw(2) << std::setfill('0') << time.seconds()       << ' '
                    << hostname << ' ' << m_tag << '[' << pid << ']' << ':'      << ' ' // HOSTNAME TAG[PID]:
                    << std::setiosflags(formatting);
        }
    }

    auto& operator<< (syslogstream& (*pf)(syslogstream&))
    {
        (*pf)(*this);
        return *this;
    }

    template <typename T,
              typename = std::enable_if_t<!std::is_pointer_v<T>>>
    auto& operator<< (const T& type)
    {
        if(m_level >= m_severity)
            static_cast<oendpointstream&>(*this) << type;
        return *this;
    }

    auto& operator<< (const char* str)
    {
        if(m_level >= m_severity)
            static_cast<oendpointstream&>(*this) << str;
        return *this;
    }

    auto& flush()
    {
        if(m_level >= m_severity)
            put('\n').flush();
        m_mutex.unlock();
        return *this;
    }

private:

    int priority(syslog::facility f, syslog::severity s)
    {
        auto p = 8;
        p *= static_cast<int>(f);
        p += static_cast<int>(s);
        return p;
    }

    syslog::facility m_facility;

    syslog::severity m_severity;

    syslog::severity m_level;

    std::string m_tag;

    std::mutex m_mutex;
};

inline auto& error(syslogstream& ss)
{
    ss.severity(syslog::severity::error);
    ss.header();
    return ss;
}

inline auto& warning(syslogstream& ss)
{
    ss.severity(syslog::severity::warning);
    ss.header();
    return ss;
}

inline auto& notice(syslogstream& ss)
{
    ss.severity(syslog::severity::notice);
    ss.header();
    return ss;
}

inline auto& info(syslogstream& ss)
{
    ss.severity(syslog::severity::info);
    ss.header();
    return ss;
}

inline auto& debug(syslogstream& ss)
{
    ss.severity(syslog::severity::debug);
    ss.header();
    return ss;
}

inline auto& flush(syslogstream& ss)
{
    ss.flush();
    return ss;
}

extern syslogstream slog;

} // namespace net
