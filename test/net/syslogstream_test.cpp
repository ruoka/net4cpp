#include <gtest/gtest.h>
#include "net/syslogstream.hpp"

using namespace std::string_literals;
using namespace net;

class NetSyslogstreamTest : public ::testing::Test
{
protected:

    void SetUp()
    {
        slog.tag("googletest");
        slog.facility(syslog::facility::local0);
        slog.level(syslog::severity::info);
    }
};

TEST_F(NetSyslogstreamTest,Debug)
{
    slog << debug << "Kaius testing "s << 123 << flush;
}

TEST_F(NetSyslogstreamTest,Info)
{
    slog << info << "Kaius testing "s << 456 << flush;
}

TEST_F(NetSyslogstreamTest,Notice)
{
    slog << notice << "Kaius testing "s << "-123-" << flush;
}

TEST_F(NetSyslogstreamTest,Warning)
{
    slog << warning << "Kaius testing "s << 789.0 << flush;
}

TEST_F(NetSyslogstreamTest,Error)
{
    slog << error << "Kaius testing "s << true << 321 << false << flush;
}
