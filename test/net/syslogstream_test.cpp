#include <gtest/gtest.h>
#include "net/syslogstream.hpp"

using namespace std::literals;
using namespace net;

class NetSyslogstreamTest : public ::testing::Test
{
protected:

    void SetUp()
    {
        slog.appname("googletest");
        slog.facility(syslog::facility::local0);
        slog.level(syslog::severity::info);
        slog.redirect(std::clog);
    }
};

TEST_F(NetSyslogstreamTest,Debug)
{
    slog << debug << "Kaius testing "s << 123 << flush;
    slog << debug("http") << "Kaius testing "s << 123 << flush;
}

TEST_F(NetSyslogstreamTest,Info)
{
    slog << info << "Kaius testing "s << 456 << flush;
    slog << info("http") << "Kaius testing "s << 123 << flush;
}

TEST_F(NetSyslogstreamTest,Notice)
{
    slog << notice << "Kaius testing "s << "+ 123 +" << flush;
    slog << notice("http") << "Kaius testing "s << "+ 123 +" << flush;
}

TEST_F(NetSyslogstreamTest,Warning)
{
    slog << warning << "Kaius testing "s << 789.0 << flush;
    slog << warning("http") << "Kaius testing "s << 789.0 << flush;
}

TEST_F(NetSyslogstreamTest,Error)
{
    slog << error << "Kaius testing "s << true << 321 << false << flush;
    slog << error("http") << "Kaius testing "s << true << 321 << false << flush;
}
