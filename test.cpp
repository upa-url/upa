#include "url.h"
#include <iostream>
// conversion
#include <codecvt>
#include <locale>


void cout_str(const wchar_t* str) {
    std::wcout << str;
}

void cout_str(const char* str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    std::wcout << convW.from_bytes(str);
}

void cout_str(const char16_t* str) {
    std::wcout << (const wchar_t*)str;
}

void cout_str(const char32_t* str) {
    for (; *str; str++)
        std::wcout.put(static_cast<wchar_t>(*str));
}


template <typename CharT>
void url_parse(const CharT* str_url, whatwg::url* base = nullptr)
{
    static char* part_name[whatwg::url::PART_COUNT] = {
        "SCHEME",
        "USERNAME",
        "PASSWORD",
        "HOST",
        "PORT",
        "PATH",
        "QUERY",
        "FRAGMENT"
    };
    
    whatwg::url url;

    cout_str(str_url);
    std::wcout << "\n";

    url.parse(str_url, base);

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    for (int part = whatwg::url::SCHEME; part < whatwg::url::PART_COUNT; part++) {
        std::string strPart = url.get_part(static_cast<whatwg::url::PartType>(part));
        if (!strPart.empty()) {
            std::wcout << part_name[part] << ": " << convW.from_bytes(strPart) << "\n";
        }
    }
}

int main()
{
    // set user-preferred locale
    setlocale(LC_ALL, "");

    url_parse("file://d:/laikina/%2e./tek%stas.txt", nullptr);
    url_parse("filesystem:http://www.example.com/temporary/", nullptr);

    url_parse("blob:550e8400-e29b-41d4-a716-446655440000#aboutABBA");
    url_parse("invalid^scheme://example.com", nullptr);

    // iš; https://github.com/whatwg/url/issues/162
    url_parse("http://example.com/%61%62%63a%2e%64%65%7e%7f%80%81");

    url_parse("mailto:vardenis@example.com", nullptr);

    const char* szUrl = "http://user:pass@klausimėlis.lt/?key=ąče#frag";
    url_parse(szUrl);
    // char16_t
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv16;
    url_parse(conv16.from_bytes(szUrl).c_str());
    // char32_t
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv32;
    url_parse(conv32.from_bytes(szUrl).c_str());
    // wchar_t
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    url_parse(convW.from_bytes(szUrl).c_str());
    // --

    url_parse("http://user:pass@klausim%c4%97lis.lt/?key=ąče#frag");
    url_parse("http://user:pass@klausim%25lis.lt/?key=ąče#frag");

    url_parse("https://username:pass@word@example.com:123/path/data?abc=ąbč&key=value&key2=value2#fragid1-ą");

    url_parse("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    url_parse("file://example.com/bandymas/#123", nullptr);

    url_parse("http://example.com:8080/bandymas/#123", nullptr);
    url_parse("http://example.com:80/bandymas/?#", nullptr);

    // base url
    whatwg::url url_base[2];
    url_base[0].parse("http://example.org/foo/bar", nullptr);
    url_base[1].parse("http://example.org/test", nullptr);

    // https://webkit.org/blog/7086/url-parsing-in-webkit/
    // http://w3c-test.org/url/url-constructor.html
    url_parse("http://f:0/c", &url_base[0]);
    url_parse("file:a", &url_base[1]);
    url_parse("file:..", &url_base[1]);
    url_parse("https://@@@example");
    url_parse("example", &url_base[1]);

    // simple_buffer testai
    void test_buffer(); test_buffer();
}

#include "buffer.h"

void test_buffer() {
    simple_buffer<char> buff;

    std::string aaa("aaabbbccc");

    buff.reserve(100);
    buff.resize(3);
    std::strcpy(buff.data(), "ABC");
    buff.shrink_to_fit();
    buff.push_back('Z');
    buff.append(aaa.cbegin(), aaa.cend());
    buff.push_back('\0');

    std::cout << buff.data();
}
