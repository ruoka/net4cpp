#pragma once

#include "std/utility.hpp"
#include "net/sender.hpp"

namespace net
{
namespace syslog
{
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

    const auto hostname = gethostname();

    const auto pid = getpid();
};

class syslogstream : public oendpointstream
{
public:

    syslogstream(oendpointstream&& s) :
        oendpointstream{std::move(s)},
        m_facility{syslog::facility::user},
        m_severity{syslog::severity::debug},
        m_level{syslog::severity::debug},
        m_tag{"syslogstream"},
        m_parent{*this}
    {}

    void facility(syslog::facility f)
    {
        m_facility = f;
    }

    void severity(syslog::severity s)
    {
        m_severity = s;
    }

    void level(syslog::severity s)
    {
        m_level = s;
    }

    void tag(const std::string& tag)
    {
        m_tag = tag;
    }

    void header()
    {
        if(m_level >= m_severity)
        {
            using namespace std;
            using namespace std::chrono;
            years YY;
            months MM;
            days DD;
            hours hh;
            minutes mm;
            seconds ss;
            milliseconds ff;
            std::tie(YY,MM,DD,hh,mm,ss,ff) = convert(system_clock::now());
            m_parent << '<' << 8 * (int)m_facility + (int)m_severity << '>' // <PRI>
                     << MM << ' '                                           // TIMESTAMP
                     << setw(2) << setfill(' ') << DD.count() << ' '
                     << setw(2) << setfill('0') << hh.count() << ':'
                     << setw(2) << setfill('0') << mm.count() << ':'
                     << setw(2) << setfill('0') << ss.count() << ' '
                     << syslog::hostname << ' '                             // HOSTNAME
                     << m_tag << '[' << syslog::pid << ']' << ':' << ' ';   // TAG[PID]:
        }
    }

    auto& flush()
    {
        if(m_level >= m_severity)
            m_parent.flush();
        return *this;
    }

    auto& operator<< (syslogstream& (*pf)(syslogstream&))
    {
        (*pf)(*this);
        return *this;
    }

    template <typename T,
              typename = std::enable_if_t<!std::is_pointer<T>::value>>
    auto& operator<< (const T& t)
    {
        if(m_level >= m_severity)
            m_parent << t;
        return *this;
    }

private:

    syslog::facility m_facility;

    syslog::severity m_severity;

    syslog::severity m_level;

    std::string m_tag;

    oendpointstream& m_parent;
};

inline auto& error(syslogstream& sl)
{
    sl.severity(syslog::severity::error);
    sl.header();
    return sl;
}

inline auto& warning(syslogstream& sl)
{
    sl.severity(syslog::severity::warning);
    sl.header();
    return sl;
}

inline auto& notice(syslogstream& sl)
{
    sl.severity(syslog::severity::notice);
    sl.header();
    return sl;
}

inline auto& info(syslogstream& sl)
{
    sl.severity(syslog::severity::info);
    sl.header();
    return sl;
}

inline auto& debug(syslogstream& sl)
{
    sl.severity(syslog::severity::debug);
    sl.header();
    return sl;
}

inline auto& flush(syslogstream& sl)
{
    sl.flush();
    return sl;
}

static syslogstream slog{distribute("localhost","syslog")};

} // namespace net
