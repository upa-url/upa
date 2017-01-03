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
void url_testas(const CharT* str_url, whatwg::url* base = nullptr)
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

    if (url.parse(str_url, base)) {
        // print parts
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
        for (int part = whatwg::url::SCHEME; part < whatwg::url::PART_COUNT; part++) {
            std::string strPart = url.get_part(static_cast<whatwg::url::PartType>(part));
            if (!strPart.empty()) {
                std::wcout << part_name[part] << ": " << convW.from_bytes(strPart) << "\n";
            }
        }
    } else {
        std::wcout << " ^--FAILURE\n";
    }
}

int main()
{
    // set user-preferred locale
    setlocale(LC_ALL, "");

    url_testas("file://d:/laikina/%2e./tek%stas.txt", nullptr);
    url_testas("filesystem:http://www.example.com/temporary/", nullptr);

    url_testas("blob:550e8400-e29b-41d4-a716-446655440000#aboutABBA");
    url_testas("invalid^scheme://example.com", nullptr);

    // iš; https://github.com/whatwg/url/issues/162
    url_testas("http://example.com/%61%62%63a%2e%64%65%7e%7f%80%81");

    url_testas("mailto:vardenis@example.com", nullptr);

    const char* szUrl = "http://user:pass@klausimėlis.lt/?key=ąče#frag";
    url_testas(szUrl);
    // char16_t
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv16;
    url_testas(conv16.from_bytes(szUrl).c_str());
    // char32_t
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv32;
    url_testas(conv32.from_bytes(szUrl).c_str());
    // wchar_t
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    url_testas(convW.from_bytes(szUrl).c_str());
    // --

    url_testas("http://user:pass@klausim%c4%97lis.lt/?key=ąče#frag");
    url_testas("http://user:pass@klausim%25lis.lt/?key=ąče#frag");

    url_testas("https://username:pass@word@example.com:123/path/data?abc=ąbč&key=value&key2=value2#fragid1-ą");

    url_testas("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    url_testas("file://example.com/bandymas/#123", nullptr);

    url_testas("http://example.com:8080/bandymas/#123", nullptr);
    url_testas("http://example.com:80/bandymas/?#", nullptr);

    // base url
    whatwg::url url_base[2];
    url_base[0].parse("http://example.org/foo/bar", nullptr);
    url_base[1].parse("http://example.org/test", nullptr);

    // https://webkit.org/blog/7086/url-parsing-in-webkit/
    // http://w3c-test.org/url/url-constructor.html
    url_testas("http://f:0/c", &url_base[0]);
    url_testas("file:a", &url_base[1]);
    url_testas("file:..", &url_base[1]);
    url_testas("https://@@@example");
    url_testas("example", &url_base[1]);

    // IPv4 testai
    url_testas("http://127.1/kelias/", nullptr);
    url_testas("http://127.0.0.1/kelias/", nullptr);
    url_testas("http://12%37.0.0.1/kelias/", nullptr);
    url_testas("http://0x7f.0.0.1/kelias/", nullptr);

    // IPv6 testai
    url_testas("http://[1:2:3:4::6:7:8]/kelias/");  // rust-url bug
    url_testas("http://[1:2:3:4:5:6:7:8]/kelias/");
    url_testas("http://[1:2::7:8]/kelias/");
    url_testas("http://[1:2:3::]/kelias/");
    url_testas("http://[::6:7:8]/kelias/");
    url_testas("http://[::1.2.3.4]/");
    url_testas("http://[0::0]/");
    url_testas("http://[::]/");
    url_testas("http://[0:f:0:0:f:f:0:0]");
    // URL standard bug (see: "IPv6 parser" "10.7. If c is not the EOF code point, increase pointer by one.")
    // - praleis 'X' (ar jo vietoje bet kokį ne skaitmenį) be klaidų
    url_testas("http://[::1.2.3.4X]/");
    // must be failure:
    url_testas("http://[::1.2.3.]/");
    url_testas("http://[::1.]/");
    
    // https://quuz.org/url/ IPv6 serializer bug (no compressing trailing zeros):
    url_testas("http://[2::0]/");
    url_testas("http://[2::]/");

    // IDNA testai
    // http://www.unicode.org/reports/tr46/#Implementation_Notes
    url_testas("http://%E5%8D%81%zz.com/", nullptr);

    // simple_buffer testai
    void test_buffer(); test_buffer();
}

#include "buffer.h"

void test_buffer() {
    whatwg::simple_buffer<char, 16> buff;

    std::string aaa("aaabbbccc");
    std::string bbb("-ddeXeff=");

    buff.reserve(10);
    buff.resize(3);
    std::strcpy(buff.data(), "ABC");
    //buff.shrink_to_fit();
    buff.push_back('Z');
    buff.append(aaa.cbegin(), aaa.cend());
    buff.append(bbb.cbegin(), bbb.cend());
    buff.append(bbb.cbegin(), bbb.cend());
    buff.push_back('\0');

    std::cout << buff.data();
}
