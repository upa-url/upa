#include "url_percent_encode.h"
#include "doctest-main.h"


// encodeURIComponent(...) implementation as described in ECMAScript Language Specification (ECMA-262):
// https://tc39.es/ecma262/#sec-encodeuricomponent-uricomponent
// https://tc39.es/ecma262/#prod-uriUnescaped
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent
// Input must be valid UTF-8.
// It is used in tests to compare result with encode_url_component(...).
// According to URL specification results must match:
// https://url.spec.whatwg.org/#component-percent-encode-set

static std::string encodeURIComponent(const std::string& str) {
    std::stringstream strm;
    for (auto c : str) {
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


TEST_CASE_TEMPLATE_DEFINE("encode_url_component with ASCII input", CharT, test_encode_url_component_ascii) {
    using string_t = std::basic_string<CharT>;

    string_t inp{ 'a', '?', 'z' };
    std::string inpA{ 'a', '?', 'z' };

    for (unsigned i = 0; i < 0x80; ++i) {
        inp[1] = static_cast<CharT>(i);
        inpA[1] = static_cast<char>(i);
        CHECK(whatwg::encode_url_component(inp) == encodeURIComponent(inpA));
    }
}

TEST_CASE_TEMPLATE_INVOKE(test_encode_url_component_ascii, char, wchar_t, char16_t, char32_t);
#ifdef __cpp_char8_t
TEST_CASE_TEMPLATE_INVOKE(test_encode_url_component_ascii, char8_t);
#endif


TEST_CASE("encode_url_component with non ASCII input") {
    SUBCASE("U+FFFD") {
        CHECK(whatwg::encode_url_component(u8"\uFFFD") == "%EF%BF%BD");
        CHECK(whatwg::encode_url_component(u"\uFFFD") == "%EF%BF%BD");
        CHECK(whatwg::encode_url_component(U"\uFFFD") == "%EF%BF%BD");
    }

    SUBCASE("Surrogate pair") {
        CHECK(whatwg::encode_url_component(std::u16string{ char16_t(0xD800), char16_t(0xDFFF) }) == "%F0%90%8F%BF");
    }

    SUBCASE("Invalid code point must be raplaced with U+FFFD") {
        // invalid UTF-8
        CHECK(whatwg::encode_url_component(std::string{ 'a', char(0xC2), 'z' }) == "a%EF%BF%BDz");
        // lone high surrogate
        CHECK(whatwg::encode_url_component(std::u16string{ char16_t('a'), char16_t(0xD800), char16_t('z') }) == "a%EF%BF%BDz");
        CHECK(whatwg::encode_url_component(std::u32string{ char32_t('a'), char32_t(0xD800), char32_t('z') }) == "a%EF%BF%BDz");
        // lone low surrogate
        CHECK(whatwg::encode_url_component(std::u16string{ char16_t('a'), char16_t(0xDFFF), char16_t('z') }) == "a%EF%BF%BDz");
        CHECK(whatwg::encode_url_component(std::u32string{ char32_t('a'), char32_t(0xDFFF), char32_t('z') }) == "a%EF%BF%BDz");
    }
}


TEST_CASE("percent_decode") {
    SUBCASE("ASCII") {
        CHECK(whatwg::percent_decode("a%20z") == "a z");
    }
    SUBCASE("non ASCII") {
        CHECK(whatwg::percent_decode("a%C4%84z") == "a\xC4\x84z");
        CHECK(whatwg::percent_decode("a\xC4\x84z") == "a\xC4\x84z");
    }
    SUBCASE("invalid percent encode sequence") {
        CHECK(whatwg::percent_decode("a%z") == "a%z");
        CHECK(whatwg::percent_decode("a%%20z") == "a% z");
        CHECK(whatwg::percent_decode("a%20%z") == "a %z");
        CHECK(whatwg::percent_decode("a%C4%84%z") == "a\xC4\x84%z");
    }
    SUBCASE("invalid UTF-8") {
        CHECK(whatwg::percent_decode("a%C2z") == "a\xEF\xBF\xBDz");
        CHECK(whatwg::percent_decode("a\xC2z") == "a\xEF\xBF\xBDz");
    }
}
