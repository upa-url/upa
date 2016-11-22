#include <iostream>
#include "url.h"

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

    std::cout << str_url << "\n";
    url.parse(str_url, base);
    for (int part = whatwg::url::SCHEME; part < whatwg::url::PART_COUNT; part++) {
        std::string strPart = url.get_part(static_cast<whatwg::url::PartType>(part));
        if (!strPart.empty()) {
            std::cout << part_name[part] << ": " << strPart << "\n";
        }
    }
}

int main()
{
    url_parse("file://d:/tekstas.txt", nullptr);
    url_parse("filesystem:http://www.example/temporary/", nullptr);

    url_parse("invalid^scheme://example.com", nullptr);

    url_parse("mailto:vardenis@example.com", nullptr);

    url_parse((const char16_t*)L"http://klausimėlis.lt/?key=ąče#axd");
    url_parse("https://username:pass@word@example.com:123/path/data?abc=ąbč&key=value&key2=value2#fragid1-ą");

    url_parse("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    url_parse("file://example.com/bandymas/#123", nullptr);

    url_parse("http://example.com:8080/bandymas/#123", nullptr);
    url_parse("http://example.com:80/bandymas/#123", nullptr);
}
