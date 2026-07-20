// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;

auto stream_to_string(auto&& value)
{
    auto os = std::ostringstream{};
    os << std::forward<decltype(value)>(value);
    return os.str();
}
}

auto register_http_escape_tests()
{
    tester::bdd::scenario("HtmlEscaped empty, [net]") = [] {
        check_eq("", stream_to_string(http::html_escaped{""}));
    };

    tester::bdd::scenario("HtmlEscaped plain text, [net]") = [] {
        check_eq("NOKIA.HE.EUR", stream_to_string(http::html_escaped{"NOKIA.HE.EUR"}));
    };

    tester::bdd::scenario("HtmlEscaped special characters, [net]") = [] {
        check_eq("A&amp;B", stream_to_string(http::html_escaped{"A&B"}));
        check_eq("a&lt;b", stream_to_string(http::html_escaped{"a<b"}));
        check_eq("a&gt;b", stream_to_string(http::html_escaped{"a>b"}));
        check_eq("say &quot;hi&quot;", stream_to_string(http::html_escaped{R"(say "hi")"}));
        check_eq("it&#39;s", stream_to_string(http::html_escaped{"it's"}));
    };

    tester::bdd::scenario("HtmlEscaped combined specials, [net]") = [] {
        check_eq("&lt;&amp;&gt;&quot;&#39;",
                 stream_to_string(http::html_escaped{"<&>\"'"}));
    };

    tester::bdd::scenario("HtmlEscaped reflected payload, [net]") = [] {
        check_eq("&lt;script&gt;alert(1)&lt;/script&gt;",
                 stream_to_string(http::html_escaped{"<script>alert(1)</script>"}));
    };

    tester::bdd::scenario("HtmlEscaped stream composition, [net]") = [] {
        auto os = std::ostringstream{};
        os << "ref=" << http::html_escaped{"ACC+\"<x>"};
        check_eq("ref=ACC+&quot;&lt;x&gt;", os.str());
    };

    tester::bdd::scenario("UrlEncoded empty, [net]") = [] {
        check_eq("", stream_to_string(http::url_encoded{""}));
    };

    tester::bdd::scenario("UrlEncoded unreserved characters, [net]") = [] {
        check_eq("ABCabc09-._~", stream_to_string(http::url_encoded{"ABCabc09-._~"}));
    };

    tester::bdd::scenario("UrlEncoded reserved characters, [net]") = [] {
        check_eq("hello%20world", stream_to_string(http::url_encoded{"hello world"}));
        check_eq("a%3Db%26c", stream_to_string(http::url_encoded{"a=b&c"}));
        check_eq("%2F%3F%23", stream_to_string(http::url_encoded{"/?#"}));
    };

    tester::bdd::scenario("UrlEncoded stream composition, [net]") = [] {
        auto os = std::ostringstream{};
        os << "/cancel?ref=" << http::url_encoded{"ACC 123"};
        check_eq("/cancel?ref=ACC%20123", os.str());
    };

    tester::bdd::scenario("UrlEncoded round trip with url_decode, [net]") = [] {
        const auto raw = "ACC+ClOrd/ID&x=1"s;
        const auto encoded = stream_to_string(http::url_encoded{raw});
        check_eq("ACC%2BClOrd%2FID%26x%3D1", encoded);
        check_eq(raw, net::url_decode(encoded));
    };

    return true;
}

const auto _ = register_http_escape_tests();
