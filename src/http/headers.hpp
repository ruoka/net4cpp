#pragma once
#include <map>
#include "std/extension.hpp"
#include "net/syslogstream.hpp"

namespace http
{
using namespace net;
using namespace std::string_literals;

class headers
{
public:

    using name = std::string;

    using value = std::string;

    auto count(const name& n) {return m_values.count(n);};

    const value& operator[](const name& n) const {return m_values.at(n);}

    friend std::istream& operator >> (std::istream&, headers&);

private:

    std::map<name,value> m_values;
};

inline std::istream& operator >> (std::istream &is,  headers &hdrs)
{
    while(is.peek() != '\r')
    {
        auto name = ""s, value = ""s;
        getline(is, name, ':');
        getline(is, value);
        ext::trim(name);
        ext::trim(value);
        slog << info << "HTTP request header \"" << name << "\": \"" << value << "\"" << flush;
        hdrs.m_values.emplace(std::move(name), std::move(value));
    }
    return is;
};

} // namespace http
