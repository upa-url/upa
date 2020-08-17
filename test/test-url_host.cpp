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
