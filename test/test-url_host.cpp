// Copyright 2016-2023 Rimas Misevičius
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

TEST_SUITE("host_parser::parse_host (isNotSpecial = true)") {
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

TEST_SUITE("host_parser::parse_host (isNotSpecial = false)") {
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
        // Host length = 10 + 102 * 10 = 1030 > 1024 (simple_buffer's fixed buffer length)
        // Use "xn--" label to avoid ASCII fast path
        std::string strHost = "xn--2da.90";
        for (int i = 0; i < 102; ++i)
            strHost.append(".bcde12345");
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
        // TODO-TODO-TODO: test all errors
        CHECK(construct_url_host("") == upa::validation_errc::host_missing);
        CHECK(construct_url_host("xn--a") == upa::validation_errc::domain_to_ascii);
        CHECK(construct_url_host("0x100000000") == upa::validation_errc::ipv4_out_of_range_part);
        CHECK(construct_url_host("[1") == upa::validation_errc::ipv6_unclosed);
        CHECK(construct_url_host("a^2") == upa::validation_errc::domain_invalid_code_point);
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
