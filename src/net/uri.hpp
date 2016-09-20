#pragma once
#include <vector>
#include <experimental/string_view>

// scheme:[//[username:password@]host[:port]][/]path[?query][#fragment]

namespace net {

using string_view = std::experimental::string_view;

class uri
{
public:

template<typename T>
class property
{
public:

    operator T () const
    {
        return m_value;
    }

    operator std::string () const
    {
        return std::string{m_value};
    }

    auto operator == (const T& value) const
    {
        return m_value == value;
    }

protected:

    T m_value;

private:

    friend class uri;

    auto& operator = (T&& value)
    {
        m_value = value;
        return *this;
    }
};

template <char delim>
class indexed_property : public property<string_view>
{
public:

    auto operator [] (std::size_t idx) const
    {
        if(idx < m_index.size())
            return m_index[idx];
        else
            return string_view{};
    }

    auto begin() const
    {
        return m_index.begin();
    }

    auto end() const
    {
        return m_index.end();
    }

private:

    friend class uri;

    auto& operator = (string_view&& value)
    {
        m_value = value;

        auto pos = value.find_first_of(delim);

        while(pos != value.npos)
        {
            m_index.emplace_back(value.substr(0, pos));
            value.remove_prefix(pos + 1);
            pos = value.find_first_of(delim);
        }

        m_index.emplace_back(value.substr());

        return *this;
    }

    std::vector<string_view> m_index;
};

explicit uri(string_view string)
{
    auto position = string.npos;

    absolute = false;

    position = string.find_first_of(":@[]/?#");               // gen-delims

    if(position != string.npos && string.at(position) == ':') // scheme name
    {
        absolute = true;
        scheme = string.substr(0, position);
        string.remove_prefix(position + 1);                   // scheme:
    }

    position = string.find_first_not_of("/");

    if(position == 2)                                         // authority component
    {
        string.remove_prefix(2);                              // //

        position = string.find_first_of("/?#");
        auto authority = string.substr(0, position);
        string.remove_prefix(position);                       // authority

        position = authority.find_first_of('@');
        if(position != string.npos)
        {
            userinfo = authority.substr(0, position);
            authority.remove_prefix(position + 1);            // userinfo@
        }

        position = authority.find_last_of(':');
        if(position != string.npos)
        {
            const auto size = authority.length() - position;
            port = authority.substr(position + 1, size - 1);
            authority.remove_suffix(size);                    // :port
        }

        position = authority.length();
        host = authority.substr();                            // host
    }

    position = string.find_first_of("?#");                    // path component
    path = string.substr(0, position);
    string.remove_prefix(position);                           // path

    if(string.front() == '?')                                 // query component
    {
        string.remove_prefix(1);                              // ?
        position = string.find_first_of("#");
        query = string.substr(0, position);
        string.remove_prefix(position);                       // query
    }

    if(string.front() == '#')                                 // fragment component
    {
        string.remove_prefix(1);                              // #
        fragment = string.substr();                           // host
    }
}

uri(const std::string& str) : uri(string_view{str})
{}

property<bool> absolute;

property<string_view> scheme;

property<string_view> userinfo;

property<string_view> host;

property<string_view> port;

indexed_property<'/'> path;

indexed_property<'&'> query;

property<string_view> fragment;

};

} // namespace net

namespace std {

template<typename T>
inline auto& operator << (std::ostream& os, const net::uri::property<T>& p)
{
    os << static_cast<const T&>(p);
    return os;
}

} // namespace std
