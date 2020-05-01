#include "url.h"
#include "doctest-main.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>

// Tests based on "url-setters-stripping.any.js" file from
// https://github.com/web-platform-tests/wpt/tree/master/url
//
// Last checked for updates: 2020-05-01
//

using url_args = std::unordered_map<std::string, std::string>;

static const url_args urlDefaultArgs = {
    { "scheme", "https" },
    { "username", "username" },
    { "password", "password" },
    { "host", "host" },
    { "port", "8000" },
    { "pathname", "path" },
    { "search", "query" },
    { "hash", "fragment" }
};

static std::string urlString(url_args urlArgs) {
    // merge
    urlArgs.insert(urlDefaultArgs.begin(), urlDefaultArgs.end());

    // construct URL string
    return urlArgs["scheme"] + "://" + urlArgs["username"] + ":" + urlArgs["password"] + "@" + urlArgs["host"] + ":" + urlArgs["port"]
        + "/" + urlArgs["pathname"] + "?" + urlArgs["search"] + "#" + urlArgs["hash"];
}

static whatwg::url urlRecord(std::string scheme) {
    whatwg::url u;
    u.parse(urlString({ { "scheme", scheme } }), nullptr);
    return u;
}

static std::string get_url_property(const whatwg::url& url, const std::string& property) {
    if (property == "prtocol")
        return url.protocol();
    else if (property == "username")
        return url.username();
    else if (property == "password")
        return url.password();
    else if (property == "host")
        return url.host();
    else if (property == "hostname")
        return url.hostname();
    else if (property == "port")
        return url.port();
    else if (property == "pathname")
        return url.pathname();
    else if (property == "search")
        return url.search();
    else if (property == "hash")
        return url.hash();
    else if (property == "href")
        return url.href();
}

static void set_url_property(whatwg::url& url, const std::string& property, const std::string& value) {
    if (property == "prtocol")
        url.protocol(value);
    else if (property == "username")
        url.username(value);
    else if (property == "password")
        url.password(value);
    else if (property == "host")
        url.host(value);
    else if (property == "hostname")
        url.hostname(value);
    else if (property == "port")
        url.port(value);
    else if (property == "pathname")
        url.pathname(value);
    else if (property == "search")
        url.search(value);
    else if (property == "hash")
        url.hash(value);
}

static std::string encodeURIComponent(const std::string& str) {
    std::stringstream strm;
    for (char c : str) {
        // Not escapes: A-Z a-z 0-9 - _ . ! ~ * ' ( )
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') {
            strm << c;
        } else {
            // percent encode
            strm << '%' << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << int(c);
        }
    }
    return strm.str();
}

//
// https://github.com/web-platform-tests/wpt/blob/master/url/url-setters-stripping.any.js
//
TEST_CASE("url-setters-stripping.any.js") {

    for (const std::string scheme : { "https", "wpt++" }) {
        for (int i = 0; i < 0x20; ++i) {
            const bool stripped = i == 0x09 || i == 0x0A || i == 0x0D;

            // It turns out that user agents are surprisingly similar for these ranges so generate fewer
            // tests. If this is changed also change the logic for host below.
            if (i != 0 && i != 0x1F && !stripped) {
                continue;
            }

            std::stringstream strm;
            strm << "U+" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << i;
            const std::string cpReference = strm.str();
            const std::string cpString = { char(i) };

            // Protocol tests

            std::string scheme_to_set = scheme == "https" ? "http" : "wpt--";
            std::string scheme_expected = stripped ? scheme_to_set : scheme;
            INFO("Setting protocol with leading " << cpReference << " (" << scheme << ":)");
            {
                auto url = urlRecord(scheme);
                url.protocol(cpString + scheme_to_set);
                CHECK(url.protocol() == scheme_expected + ":");
                CHECK(url.href() == urlString({ {"scheme", scheme_expected } }));
            }
            INFO("Setting protocol with " << cpReference << " before inserted colon (" << scheme << ":)");
            {
                auto url = urlRecord(scheme);
                url.protocol(scheme_to_set + cpString);
                CHECK(url.protocol() == scheme_expected + ":");
                CHECK(url.href() == urlString({ {"scheme", scheme_expected } }));
            }
            // cannot test protocol with trailing as the algorithm inserts a colon before proceeding

            // These do no stripping

            struct TestData {
                std::string type, expectedPart, input;
            };

            for (const std::string property : { "username", "password" }) {
                for (const TestData& td : {
                    TestData{"leading", encodeURIComponent(cpString) + "test", cpString + "test"},
                    TestData{"middle", "te" + encodeURIComponent(cpString) + "st", "te" + cpString + "st"},
                    TestData{"trailing", "test" + encodeURIComponent(cpString), "test" + cpString}
                    })
                {
                    INFO("Setting " << property << " with " << td.type << "  " << cpReference << " (" << scheme << ":)");
                    auto url = urlRecord(scheme);
                    set_url_property(url, property, td.input);
                    CHECK(get_url_property(url, property) == td.expectedPart);
                    CHECK(url.href() == urlString({ {"scheme", scheme} , {property, td.expectedPart} }));
                }
            }

            for (const TestData& td : {
                TestData{"leading", (scheme == "https" ? cpString : encodeURIComponent(cpString)) + "test", cpString + "test"},
                TestData{"middle", "te" + (scheme == "https" ? cpString : encodeURIComponent(cpString)) + "st", "te" + cpString + "st"},
                TestData{"trailing", "test" + (scheme == "https" ? cpString : encodeURIComponent(cpString)), "test" + cpString}
                })
            {
                const std::string expected = i == 0x00 ? "host" : stripped ? "test" : td.expectedPart;

                INFO("Setting host with " << td.type << "  " << cpReference << " (" << scheme << ":)");
                {
                    auto url = urlRecord(scheme);
                    url.host(td.input);
                    CHECK(url.host() == expected + ":8000");
                    CHECK(url.href() == urlString({ {"scheme", scheme }, { "host", expected } }));

                }

                INFO("Setting hostname with " << td.type << "  " << cpReference << " (" << scheme << ":)");
                {
                    auto url = urlRecord(scheme);
                    url.hostname(td.input);
                    CHECK(url.hostname() == expected);
                    CHECK(url.href() == urlString({ {"scheme", scheme }, { "host", expected } }));
                }
            }

            INFO("Setting port with leading " << cpReference << " (" << scheme << ":)");
            {
                const std::string expected = stripped ? "9000" : "8000";
                auto url = urlRecord(scheme);
                url.port(cpString + "9000");
                CHECK(url.port() == expected);
                CHECK(url.href() == urlString({ {"scheme", scheme }, { "port", expected } }));
            }

            INFO("Setting port with middle " << cpReference << " (" << scheme << ":)");
            {
                const std::string expected = stripped ? "9000" : "90";
                auto url = urlRecord(scheme);
                url.port("90" + cpString + "00");
                CHECK(url.port() == expected);
                CHECK(url.href() == urlString({ {"scheme", scheme }, { "port", expected } }));
            }

            INFO("Setting port with trailing " << cpReference << " (" << scheme << ":)");
            {
                const std::string expected = "9000";
                auto url = urlRecord(scheme);
                url.port("9000" + cpString);
                CHECK(url.port() == expected);
                CHECK(url.href() == urlString({ {"scheme", scheme }, { "port", expected } }));
            }

            struct PropSep {
                std::string property, separator;
            };

            for (const PropSep& ps : {
                PropSep{"pathname", "/"},
                PropSep{"search", "?"},
                PropSep{"hash", "#"}
                })
            {
                for (const TestData& td : {
                    TestData{"leading", encodeURIComponent(cpString) + "test", cpString + "test"},
                    TestData{"middle", "te" + encodeURIComponent(cpString) + "st", "te" + cpString + "st"},
                    TestData{"trailing", "test" + encodeURIComponent(cpString), "test" + cpString}
                    })
                {
                    INFO("Setting " << ps.property << " with " << td.type << " " << cpReference << " (" << scheme << ":)");

                    const std::string  expected = stripped ? "test" : td.expectedPart;
                    auto url = urlRecord(scheme);
                    set_url_property(url, ps.property, td.input);
                    CHECK(get_url_property(url, ps.property) == ps.separator + expected);
                    CHECK(url.href() == urlString({ {"scheme", scheme }, { ps.property, expected } }));
                }
            }
        }
    }
}
