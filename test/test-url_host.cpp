#include "url_host.h"
#include "doctest-main.h"


class host_out : public whatwg::host_output {
public:
    std::string host;
    whatwg::HostType host_type = whatwg::HostType::Empty;

    std::string& hostStart() override {
        return host;
    }
    void hostDone(whatwg::HostType ht) override {
        host_type = ht;
    }
};

TEST_SUITE("host_parser::parse_host (isNotSpecial = true)") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == whatwg::url_result::Ok);
        CHECK(out.host == "");
        CHECK(out.host_type == whatwg::HostType::Empty);
    }

    TEST_CASE("HostType::Opaque") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == whatwg::url_result::Ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == whatwg::HostType::Opaque);
    }

    TEST_CASE("HostType::IPv6") {
        const std::string strHost = "[1::0]";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), true, out) == whatwg::url_result::Ok);
        CHECK(out.host == "[1::]");
        CHECK(out.host_type == whatwg::HostType::IPv6);
    }
}

TEST_SUITE("host_parser::parse_host (isNotSpecial = false)") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == whatwg::url_result::EmptyHost);
        CHECK(out.host == "");
        CHECK(out.host_type == whatwg::HostType::Empty);
    }

    TEST_CASE("HostType::Domain") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == whatwg::url_result::Ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == whatwg::HostType::Domain);
    }

    TEST_CASE("HostType::IPv4") {
        const std::string strHost = "127.0.0.1";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == whatwg::url_result::Ok);
        CHECK(out.host == "127.0.0.1");
        CHECK(out.host_type == whatwg::HostType::IPv4);
    }

    TEST_CASE("HostType::IPv6") {
        const std::string strHost = "[1::0]";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_host(strHost.data(), strHost.data() + strHost.length(), false, out) == whatwg::url_result::Ok);
        CHECK(out.host == "[1::]");
        CHECK(out.host_type == whatwg::HostType::IPv6);
    }
}

TEST_SUITE("host_parser::parse_opaque_host") {
    TEST_CASE("HostType::Empty") {
        const std::string strHost = "";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_opaque_host(strHost.data(), strHost.data() + strHost.length(), out) == whatwg::url_result::Ok);
        CHECK(out.host == "");
        CHECK(out.host_type == whatwg::HostType::Empty);
    }

    TEST_CASE("HostType::Opaque") {
        const std::string strHost = "host";
        host_out out;

        REQUIRE(whatwg::host_parser::parse_opaque_host(strHost.data(), strHost.data() + strHost.length(), out) == whatwg::url_result::Ok);
        CHECK(out.host == "host");
        CHECK(out.host_type == whatwg::HostType::Opaque);
    }
}


template <class ...Args>
whatwg::url_result construct_url_host(Args&&... args) {
    try {
        whatwg::url_host h{ std::forward<Args>(args)... };
        return whatwg::url_result::Ok;
    }
    catch (whatwg::url_error& ex) {
        return ex.result();
    }
}

TEST_SUITE("url_host") {
    TEST_CASE("Invalid host") {
        CHECK(construct_url_host("") == whatwg::url_result::EmptyHost);
        CHECK(construct_url_host("xn--a") == whatwg::url_result::IdnaError);
        CHECK(construct_url_host("0x100000000") == whatwg::url_result::InvalidIpv4Address);
        CHECK(construct_url_host("[1") == whatwg::url_result::InvalidIpv6Address);
        CHECK(construct_url_host("a^2") == whatwg::url_result::InvalidDomainCharacter);
    }

    TEST_CASE("HostType::Domain") {
        whatwg::url_host h{ "host" };
        CHECK(h.to_string() == "host");
        CHECK(h.type() == whatwg::HostType::Domain);
    }

    TEST_CASE("HostType::IPv4") {
        whatwg::url_host h{ "127.0.0.1" };
        CHECK(h.to_string() == "127.0.0.1");
        CHECK(h.type() == whatwg::HostType::IPv4);
    }

    TEST_CASE("HostType::IPv6") {
        whatwg::url_host h{ "[1::0]" };
        CHECK(h.to_string() == "[1::]");
        CHECK(h.type() == whatwg::HostType::IPv6);
    }

    TEST_CASE("Copy constructor") {
        whatwg::url_host h{ "example.org" };
        CHECK(h.to_string() == "example.org");
        CHECK(h.type() == whatwg::HostType::Domain);

        whatwg::url_host hc{ h };
        CHECK(hc.to_string() == "example.org");
        CHECK(hc.type() == whatwg::HostType::Domain);
    }

    TEST_CASE("Move constructor") {
        whatwg::url_host h{ "[1:2::3]" };
        CHECK(h.to_string() == "[1:2::3]");
        CHECK(h.type() == whatwg::HostType::IPv6);

        whatwg::url_host hm{ std::move(h) };
        CHECK(hm.to_string() == "[1:2::3]");
        CHECK(hm.type() == whatwg::HostType::IPv6);
    }
}
