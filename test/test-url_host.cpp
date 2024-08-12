// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url_host.h"
#include "doctest-main.h"


// Test host_parser class static functions:
// * parse_host
// * parse_opaque_host

class host_out : public upa::host_output {
public:
    std::string host;
    upa::HostType host_type = upa::HostType::Empty;

    std::string& hostStart() override {
        return host;
    }
    void hostDone(upa::HostType ht) override {
        host_type = ht;
    }
};

static std::string long_host() {
    // Host length = 10 + 102 * 10 = 1030 > 1024 (simple_buffer's fixed buffer length)
    // Use "xn--" label to avoid ASCII fast path
    std::string strHost = "xn--2da.90";
    for (int i = 0; i < 102; ++i)
        strHost.append(".bcde12345");
    return strHost;
}

TEST_SUITE("host_parser::parse_host (is_opaque = true)") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == upa::validation_errc::ok);
        CHECK(out.host == "");
        CHECK(out.host_type == upa::HostType::Empty);
    }

    TEST_CASE("HostType::Opaque") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == upa::validation_errc::ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == upa::HostType::Opaque);
    }

    TEST_CASE("HostType::IPv6") {
        const std::string strHost = "[1::0]";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == upa::validation_errc::ok);
        CHECK(out.host == "[1::]");
        CHECK(out.host_type == upa::HostType::IPv6);
    }
}

TEST_SUITE("host_parser::parse_host (is_opaque = false)") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == upa::validation_errc::host_missing);
        CHECK(out.host == "");
        CHECK(out.host_type == upa::HostType::Empty);
    }

    TEST_CASE("HostType::Domain") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == upa::validation_errc::ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == upa::HostType::Domain);
    }

    TEST_CASE("HostType::Domain with long host") {
        std::string strHost = long_host();
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == upa::validation_errc::ok);
        CHECK(out.host == strHost);
        CHECK(out.host_type == upa::HostType::Domain);
    }

    TEST_CASE("HostType::IPv4") {
        const std::string strHost = "127.0.0.1";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == upa::validation_errc::ok);
        CHECK(out.host == "127.0.0.1");
        CHECK(out.host_type == upa::HostType::IPv4);
    }

    TEST_CASE("HostType::IPv6") {
        const std::string strHost = "[1::0]";
        host_out out;

        REQUIRE(upa::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == upa::validation_errc::ok);
        CHECK(out.host == "[1::]");
        CHECK(out.host_type == upa::HostType::IPv6);
    }
}

TEST_SUITE("host_parser::parse_opaque_host") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(upa::host_parser::parse_opaque_host(strHost.data(), strHost.data() + strHost.length(), out) == upa::validation_errc::ok);
        CHECK(out.host == "");
        CHECK(out.host_type == upa::HostType::Empty);
    }

    TEST_CASE("HostType::Opaque") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(upa::host_parser::parse_opaque_host(strHost.data(), strHost.data() + strHost.length(), out) == upa::validation_errc::ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == upa::HostType::Opaque);
    }
}


// Test upa::url_host class

template <class ...Args>
upa::validation_errc construct_url_host(Args&&... args) {
    try {
        upa::url_host h{ std::forward<Args>(args)... };
        return upa::validation_errc::ok;
    }
    catch (upa::url_error& ex) {
        return ex.result();
    }
}

TEST_SUITE("url_host") {
    TEST_CASE("Invalid host") {
        // IDNA
        // https://url.spec.whatwg.org/#validation-error-domain-to-ascii
        CHECK(construct_url_host("xn--a") == upa::validation_errc::domain_to_ascii); // MY

        // Host parsing (only special hosts)
        // https://url.spec.whatwg.org/#domain-invalid-code-point
        CHECK(construct_url_host("exa#mple.org") == upa::validation_errc::domain_invalid_code_point);
        // https://url.spec.whatwg.org/#ipv4-too-many-parts
        CHECK(construct_url_host("1.2.3.4.5") == upa::validation_errc::ipv4_too_many_parts);
        // https://url.spec.whatwg.org/#ipv4-non-numeric-part
        CHECK(construct_url_host("test.42") == upa::validation_errc::ipv4_non_numeric_part);
        // https://url.spec.whatwg.org/#ipv4-out-of-range-part
        CHECK(construct_url_host("255.255.4000.1") == upa::validation_errc::ipv4_out_of_range_part);
        CHECK(construct_url_host("0x100000000") == upa::validation_errc::ipv4_out_of_range_part); // MY
        // https://url.spec.whatwg.org/#ipv6-unclosed
        CHECK(construct_url_host("[::1") == upa::validation_errc::ipv6_unclosed);
        CHECK(construct_url_host("[1") == upa::validation_errc::ipv6_unclosed); // MY
        // https://url.spec.whatwg.org/#ipv6-invalid-compression
        CHECK(construct_url_host("[:1]") == upa::validation_errc::ipv6_invalid_compression);
        // https://url.spec.whatwg.org/#ipv6-too-many-pieces
        CHECK(construct_url_host("[1:2:3:4:5:6:7:8:9]") == upa::validation_errc::ipv6_too_many_pieces);
        // https://url.spec.whatwg.org/#ipv6-multiple-compression
        CHECK(construct_url_host("[1::1::1]") == upa::validation_errc::ipv6_multiple_compression);
        // https://url.spec.whatwg.org/#ipv6-invalid-code-point
        CHECK(construct_url_host("[1:2:3!:4]") == upa::validation_errc::ipv6_invalid_code_point);
        CHECK(construct_url_host("[1:2:3:]") == upa::validation_errc::ipv6_invalid_code_point);
        // https://url.spec.whatwg.org/#ipv6-too-few-pieces
        CHECK(construct_url_host("[1:2:3]") == upa::validation_errc::ipv6_too_few_pieces);
        // https://url.spec.whatwg.org/#ipv4-in-ipv6-too-many-pieces
        CHECK(construct_url_host("[1:1:1:1:1:1:1:127.0.0.1]") == upa::validation_errc::ipv4_in_ipv6_too_many_pieces);
        // https://url.spec.whatwg.org/#ipv4-in-ipv6-invalid-code-point
        CHECK(construct_url_host("[ffff::.0.0.1]") == upa::validation_errc::ipv4_in_ipv6_invalid_code_point);
        CHECK(construct_url_host("[ffff::127.0.xyz.1]") == upa::validation_errc::ipv4_in_ipv6_invalid_code_point);
        CHECK(construct_url_host("[ffff::127.0xyz]") == upa::validation_errc::ipv4_in_ipv6_invalid_code_point);
        CHECK(construct_url_host("[ffff::127.00.0.1]") == upa::validation_errc::ipv4_in_ipv6_invalid_code_point);
        CHECK(construct_url_host("[ffff::127.0.0.1.2]") == upa::validation_errc::ipv4_in_ipv6_invalid_code_point);
        // https://url.spec.whatwg.org/#ipv4-in-ipv6-out-of-range-part
        CHECK(construct_url_host("[ffff::127.0.0.4000]") == upa::validation_errc::ipv4_in_ipv6_out_of_range_part);
        // https://url.spec.whatwg.org/#ipv4-in-ipv6-too-few-parts
        CHECK(construct_url_host("[ffff::127.0.0]") == upa::validation_errc::ipv4_in_ipv6_too_few_parts);

        // Empty host
        CHECK(construct_url_host("") == upa::validation_errc::host_missing);
    }

    TEST_CASE("HostType::Domain") {
        upa::url_host h{ "host" };
        CHECK(h.to_string() == "host");
        CHECK(h.type() == upa::HostType::Domain);
    }

    TEST_CASE("HostType::IPv4") {
        upa::url_host h{ "127.0.0.1" };
        CHECK(h.to_string() == "127.0.0.1");
        CHECK(h.type() == upa::HostType::IPv4);
    }

    TEST_CASE("HostType::IPv6") {
        upa::url_host h{ "[1::0]" };
        CHECK(h.to_string() == "[1::]");
        CHECK(h.type() == upa::HostType::IPv6);
    }

    TEST_CASE("Copy constructor") {
        upa::url_host h{ "example.org" };
        CHECK(h.to_string() == "example.org");
        CHECK(h.type() == upa::HostType::Domain);

        upa::url_host hc{ h };
        CHECK(hc.to_string() == "example.org");
        CHECK(hc.type() == upa::HostType::Domain);
    }

    TEST_CASE("Move constructor") {
        upa::url_host h{ "[1:2::3]" };
        CHECK(h.to_string() == "[1:2::3]");
        CHECK(h.type() == upa::HostType::IPv6);

        upa::url_host hm{ std::move(h) };
        CHECK(hm.to_string() == "[1:2::3]");
        CHECK(hm.type() == upa::HostType::IPv6);
    }

    TEST_CASE("Copy assignment") {
        const upa::url_host h{ "example.org" };
        CHECK(h.to_string() == "example.org");
        CHECK(h.type() == upa::HostType::Domain);

        upa::url_host hc{ "[1::2]" };
        hc = h;
        CHECK(hc.to_string() == "example.org");
        CHECK(hc.type() == upa::HostType::Domain);
    }

    TEST_CASE("Move assignment") {
        upa::url_host hm{ "[1::2]" };
        hm = upa::url_host{ "1.2.3.4" };
        CHECK(hm.to_string() == "1.2.3.4");
        CHECK(hm.type() == upa::HostType::IPv4);
    }
}

#if 0

// Test upa::domain_to_unicode function

TEST_SUITE("domain_to_unicode") {
    TEST_CASE("Valid input") {
        upa::simple_buffer<char> output;
        CHECK(upa::domain_to_unicode("abc", 3, output) == upa::validation_errc::ok);
        CHECK(upa::string_view(output.data(), output.size()) == "abc");
    }

    TEST_CASE("Valid long input") {
        const std::string input = long_host();
        upa::simple_buffer<char> output;
        CHECK(upa::domain_to_unicode(input.data(), input.length(), output) == upa::validation_errc::ok);
    }

    TEST_CASE("Invalid input") {
        upa::simple_buffer<char> output;
        // IDNA errors are not failures for this function, so it returns `ok`
        CHECK(upa::domain_to_unicode("xn--a.op", 8, output) == upa::validation_errc::ok);
    }
}

#endif
