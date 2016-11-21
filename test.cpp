#include <iostream>
#include "url.hpp"

int main()
{
    whatwg::url url;

    url.parse("file://d:/tekstas.txt", nullptr);
    url.parse("filesystem:http://www.example/temporary/", nullptr);

    url.parse("invalid^scheme://example.com", nullptr);

    url.parse("mailto:vardenis@example.com", nullptr);

    url.parse("https://username:pass@word@example.com:123/path/data?key=value&key2=value2#fragid1", nullptr);

    url.parse("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    std::string str_url("http://example.com/bandymas/#123");
    url.parse(str_url, nullptr);

    str_url = "file://example.com/bandymas/#123";
    url.parse(str_url, nullptr);
}
