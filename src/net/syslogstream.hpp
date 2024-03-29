#pragma once

// https://www.rfc-editor.org/rfc/rfc3164.txt
// https://www.rfc-editor.org/rfc/rfc5424.txt

#include <mutex>
#include <iomanip>
#include <array>
#include <type_traits>
#include "net/endpointstream.hpp"
#include "std/extension.hpp"

namespace net {
namespace syslog {

    const int version = 1;

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

    struct helper
    {
        syslog::severity severity;
        std::string_view msgid;
    };
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
        m_appname{"syslogstream"},
        m_msgid{"-"},
        m_structured_data{"-"},
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

    template<std::unsigned_integral T>
    void facility(T f)
    {
        m_facility = static_cast<syslog::facility>(f);
    }

    void level(syslog::severity s)
    {
        m_level = s;
    }

    template<std::unsigned_integral T>
    void level(T s)
    {
        m_level = static_cast<syslog::severity>(s);
    }

    void appname(std::string_view app)
    {
        m_appname = app;
    }

    [[deprecated("Use appname() instead.")]]
    void tag(std::string_view app)
    {
        m_appname = app;
    }

    auto& operator << (syslog::helper&& h)
    {
        m_mutex.lock();
        m_severity = h.severity;
        m_msgid = h.msgid;
        header();
        return *this;
    }

    auto& operator << (syslogstream& (*pf)(syslogstream&))
    {
        (*pf)(*this);
        return *this;
    }

    template <typename T> requires (not std::is_pointer_v<T> and not std::same_as<std::remove_cvref_t<T>,syslog::helper>)
    auto& operator << (T&& type)
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

    void header()
    {
        // PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID
        using namespace std;
        static const auto hostname = syslog::gethostname();
        static const auto procid = syslog::getpid();

        if(m_level >= m_severity)
        {
            const auto current_time = std::chrono::system_clock::now(); // FIXME use utc_clock
            const auto midnight = std::chrono::floor<std::chrono::days>(current_time);
            const auto date = std::chrono::year_month_day{midnight};
            const auto time = std::chrono::hh_mm_ss{current_time - midnight};
            const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.subseconds());
            const auto formatting = flags();
            // <PRI>1 YYYY-MM-DDThh:mm:ss.fffZ
            static_cast<oendpointstream&>(*this)
                    << std::resetiosflags(formatting)
                    << '<' << priority(m_facility, m_severity) << '>'                      // <PRI>
                    << syslog::version << ' '                                              // 1
                    << std::setw(4) << std::setfill('0') << (int)date.year()        << '-' // YYYY-
                    << std::setw(2) << std::setfill('0') << (unsigned)date.month()  << '-' // MM-
                    << std::setw(2) << std::setfill('0') << (unsigned)date.day()    << 'T' // DDT
                    << std::setw(2) << std::setfill('0') << time.hours().count()    << ':' // hh:
                    << std::setw(2) << std::setfill('0') << time.minutes().count()  << ':' // mm:
                    << std::setw(2) << std::setfill('0') << time.seconds().count()  << '.' // ss.
                    << std::setw(3) << std::setfill('0') << milliseconds.count()    << 'Z' // fffZ
                    // SP HOSTNAME SP APP-NAME SP PROCID SP MSGID SP BOM
                    << ' ' << hostname << ' ' << m_appname << ' ' << procid << ' ' << m_msgid << ' ' << m_structured_data << " BOM"
                    << std::setiosflags(formatting);
        }
    }

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

    std::string m_appname;

    std::string m_msgid;

    std::string m_structured_data;

    std::mutex m_mutex;
};

inline syslogstream& error(syslogstream& ss)
{
    ss << syslog::helper{syslog::severity::error,"-"};
    return ss;
}

inline syslog::helper error(std::string_view msgid = "-")
{
    return {syslog::severity::error,msgid};
}

inline syslogstream& warning(syslogstream& ss)
{
    ss << syslog::helper{syslog::severity::warning,"-"};
    return ss;
}

inline syslog::helper warning(std::string_view msgid = "-")
{
    return {syslog::severity::warning,msgid};
}

inline syslogstream& notice(syslogstream& ss)
{
    ss << syslog::helper{syslog::severity::notice,"-"};
    return ss;
}

inline syslog::helper notice(std::string_view msgid = "-")
{
    return {syslog::severity::notice,msgid};
}

inline syslogstream& info(syslogstream& ss)
{
    ss << syslog::helper{syslog::severity::info,"-"};
    return ss;
}

inline syslog::helper info(std::string_view msgid = "-")
{
    return {syslog::severity::info,msgid};
}

inline syslogstream& debug(syslogstream& ss)
{
    ss << syslog::helper{syslog::severity::debug,"-"};
    return ss;
}

inline syslog::helper debug(std::string_view msgid = "-")
{
    return {syslog::severity::debug,msgid};
}

inline auto& flush(syslogstream& ss)
{
    ss.flush();
    return ss;
}

extern syslogstream slog;

} // namespace net
