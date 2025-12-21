#pragma once
#include <map>
#include "std/extension.hpp"

namespace http
{
using namespace net;
using namespace std::string_literals;

class headers
{
public:

    using name = std::string;

    using value = std::string;

    auto contains(const name& n) const {return m_values.contains(n);};

    const value& operator[](const name& n) const {return m_values.at(n);}
    
    value& operator[](const name& n) {return m_values[n];}
    
    void set(const name& n, const value& v)
    {
        auto lower_name = n;
        ext::to_lower(lower_name);
        m_values[lower_name] = v;
    }

    friend std::istream& operator >> (std::istream&, headers&);

    friend std::ostream& operator << (std::ostream&, const headers&);

private:

    std::map<name,value> m_values;
};

inline std::istream& operator >> (std::istream &is,  headers &hdrs)
{
    while(is.good() and is.peek() != '\r')
    {
        auto name = ""s, value = ""s;
        getline(is, name, ':');
        ext::trim(name);
        getline(is, value);
        ext::trim(value);
        ext::to_lower(name);
        hdrs.m_values.emplace(std::move(name), std::move(value));
    }
    return is;
};

inline std::ostream& operator << (std::ostream &os, const headers &hdrs)
{
    for(const auto &[name,value] : hdrs.m_values)
        os << name << ": " << value << crlf;
    return os;
};

} // namespace http
