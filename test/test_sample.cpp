#include "url.h"
#include <fstream>
#include <iostream>
// conversion
#include <codecvt>
#include <locale>
// json
#include "json_writer/json_writer.h"
// https://github.com/kazuho/picojson
# include "picojson/picojson.h"


template <class ...Args>
std::wstring to_wstr(Args&& ...args) {
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convW;
    return convW.from_bytes(std::forward<Args>(args)...);
}

void cout_str(const wchar_t* str) {
    std::wcout << str;
}

void cout_str(const char* str) {
    std::wcout << to_wstr(str);
}

void cout_str(const char16_t* str) {
    std::wcout << (const wchar_t*)str;
}

void cout_str(const char32_t* str) {
    for (; *str; str++)
        std::wcout.put(static_cast<wchar_t>(*str));
}

template <class T>
void cout_name_str(const char* name, T&& str) {
    if (!str.empty()) {
        std::wcout << name << ": " << to_wstr(std::forward<T>(str)) << "\n";
    }
}

void cout_url(const whatwg::url& url) {
    static const char* part_name[whatwg::url::PART_COUNT] = {
        "SCHEME",
        nullptr,
        "USERNAME",
        "PASSWORD",
        nullptr,
        "HOST",
        "PORT",
        "PATH",
        "QUERY",
        "FRAGMENT"
    };

    cout_name_str("HREF", url.href());
    cout_name_str("origin", url.origin());

    // print parts
    for (int part = whatwg::url::SCHEME; part < whatwg::url::PART_COUNT; part++) {
        if (!part_name[part]) continue;

        if (part == whatwg::url::PATH) {
            cout_name_str("path", url.path());
            cout_name_str("pathname", url.pathname());
            continue;
        }

        if (part == whatwg::url::HOST) {
            const char* szHostType;
            if (url.is_null(static_cast<whatwg::url::PartType>(part))) {
                szHostType = "null";
            } else {
                switch (url.host_type()) {
                case whatwg::HostType::Empty: szHostType = "Empty"; break;
                case whatwg::HostType::Opaque: szHostType = "Opaque"; break;
                case whatwg::HostType::Domain: szHostType = "Domain"; break;
                case whatwg::HostType::IPv4: szHostType = "IPv4"; break;
                case whatwg::HostType::IPv6: szHostType = "IPv6"; break;
                }
            }
            std::wcout << "host_type: " << szHostType << "\n";
        }

        cout_name_str(part_name[part], url.get_part_view(static_cast<whatwg::url::PartType>(part)));
    }
}

void cout_url_eol(const whatwg::url& url) {
    cout_url(url);
    std::wcout << std::endl;
}

template <typename CharT>
void url_testas(const CharT* str_url, whatwg::url* base = nullptr)
{
    // source data
    cout_str(str_url);  std::wcout << "\n";
    if (base) {
        std::wcout << "BASE: " << to_wstr(base->href()) << "\n";
    }

    // url parse result
    whatwg::url url;
    if (whatwg::success(url.parse(str_url, base))) {
        // serialized
        cout_url(url);
    } else {
        std::wcout << " ^--FAILURE\n";
    }
    std::wcout << std::endl;
}

template <typename CharT>
void url_testas(const CharT* str_url, const CharT* str_base)
{
    if (str_base) {
        whatwg::url url_base;
        if (whatwg::success(url_base.parse(str_base, nullptr))) {
            url_testas(str_url, &url_base);
        } else {
            cout_str(str_base);  std::wcout << "\n";
            std::wcout << " ^-BASE-PARSE-FAILURE\n";
        }
    } else {
        url_testas(str_url);
    }
}

template <typename CharT>
void url_parse_to_json(json_writer& json, const CharT* str_url, whatwg::url* base = nullptr)
{
    json.object_start();

    json.name("input"); json.value(str_url);
    if (base) {
        json.name("base"); json.value(base->href());
    }

    // url parse result
    whatwg::url url;
    if (whatwg::success(url.parse(str_url, base))) {
        json.name("href");     json.value(url.href());
        json.name("origin");   json.value(url.origin()); // ne visada reikia
        json.name("protocol"); json.value(url.protocol());
        json.name("username"); json.value(url.username());
        json.name("password"); json.value(url.password());
        json.name("host");     json.value(url.host());
        json.name("hostname"); json.value(url.hostname());
        json.name("port");     json.value(url.port());
        json.name("pathname"); json.value(url.pathname());
        json.name("search");   json.value(url.search());
        json.name("hash");     json.value(url.hash());
    } else {
        json.name("failure");  json.value_bool(true);
    }

    json.object_end();
}

class SamplesOutput {
public:
    virtual bool open() { return true; };
    virtual void close() {};
    virtual void comment(const char* sz) {
        std::wcout << sz << std::endl;
        while (*sz) std::wcout << '~', sz++;
        std::wcout << std::endl;
    }
    virtual void output(const char* str_url, whatwg::url* base) {
        url_testas(str_url, base);
    }
};

class SamplesOutputJson : public SamplesOutput {
public:
    SamplesOutputJson(std::string&& fname)
        : fname_(std::forward<std::string>(fname))
        , json_(fout_, 2)
    {}

    bool open() override {
        fout_.open(fname_, std::ios_base::out | std::ios_base::binary);
        if (!fout_.is_open()) {
            std::cerr << "Can't create results file: " << fname_ << std::endl;
            return false;
        }
        json_.array_start();
        return true;
    };

    void close() override {
        json_.array_end();
    }

    virtual void comment(const char* sz) {
        json_.value(sz);
    }
    void output(const char* str_url, whatwg::url* base) override {
        url_parse_to_json(json_, str_url, base);
    }
private:
    std::string fname_;
    std::ofstream fout_;
    json_writer json_;
};

/*
 * URL samples reader
 *
 * File format:

COMMENT:<comment>
BASE:<base URL>
URL:
<url1>
"<url2 as JSON string>"

SET:<setter name>
url:<URL to parse>
val:<new value>

**/

bool read_setter(std::ifstream& file, const char* name, const char* name_end);

void read_samples(const char* file_name, SamplesOutput& out)
{
    std::cout << "========== " << file_name << " ==========\n";
    std::ifstream file(file_name, std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Can't open samples file: " << file_name << std::endl;
        return;
    }

    if (!out.open())
        return;

    enum class State {
        header, url
    } state = State::header;
    whatwg::url url_base;

    std::string line;
    while (std::getline(file, line)) {
        switch (state) {
        case State::header: {
            bool ok = true;
            auto icolon = line.find(':');
            if (icolon != line.npos) {
                if (line.compare(0, icolon, "BASE") == 0) {
                    const auto res = url_base.parse(line.data() + icolon + 1, line.data() + line.length(), nullptr);
                    ok = whatwg::success(res);
                } else if (line.compare(0, icolon, "COMMENT") == 0) {
                    out.comment(line.data() + icolon + 1);
                } else if (line.compare(0, icolon, "URL") == 0) {
                    state = State::url;
                } else if (line.compare(0, icolon, "SET") == 0) {
                    if (!read_setter(file, line.data() + icolon + 1, line.data() + line.length()))
                        return;
                } else {
                    ok = false;
                }
            } else {
                ok = false;
            }
            if (!ok) {
                std::cerr << "Error in header" << std::endl;
                return;
            }
            break;
        }
        case State::url: {
            if (line.length() > 0) {
                if (line[0] == '"') {
                    // parse JSON string
                    picojson::value v;
                    std::string err = picojson::parse(v, line);
                    if (!err.empty()) {
                        std::cerr << "Skip invalid line: " << line << std::endl;
                        continue;
                    }
                    line = v.get<std::string>();
                }
                out.output(line.c_str(), (url_base.href().empty() ? nullptr : &url_base));
            } else {
                state = State::header;
                url_base.clear();
            }
            break;
        }
        }
    }

    out.close();
}

inline static void AsciiTrimWhiteSpace(const char*& first, const char*& last) {
    auto ascii_ws = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
    // trim space
    while (first < last && ascii_ws(*first)) first++;
    while (first < last && ascii_ws(*(last - 1))) last--;
}

bool read_setter(std::ifstream& file, const char* name, const char* name_end) {
    AsciiTrimWhiteSpace(name, name_end);
    std::string strName(name, name_end);

    whatwg::url url;

    std::string line;
    bool ok = true;
    while (std::getline(file, line)) {
        if (line.length() == 0) break;
        auto icolon = line.find(':');
        if (icolon != line.npos) {
            const char* val = line.data() + icolon + 1;
            const char* val_end = line.data() + line.length();
            if (line.compare(0, icolon, "url") == 0) {
                std::cout << "URL=";
                std::cout.write(val, val_end - val);
                std::cout << std::endl;
                ok = whatwg::success(url.parse(val, val_end, nullptr));
            } else if (line.compare(0, icolon, "val") == 0) {
                // set value
                if (strName == "protocol") {
                    url.protocol(val, val_end);
                } else if (strName == "username") {
                    url.username(val, val_end);
                } else if (strName == "password") {
                    url.password(val, val_end);
                } else if (strName == "host") {
                    url.host(val, val_end);
                } else if (strName == "hostname") {
                    url.hostname(val, val_end);
                } else if (strName == "port") {
                    url.port(val, val_end);
                } else if (strName == "pathname") {
                    url.pathname(val, val_end);
                } else if (strName == "search") {
                    url.search(val, val_end);
                } else if (strName == "hash") {
                    url.hash(val, val_end);
                } else {
                    std::cerr << "Unknown setter: " << strName << std::endl;
                    return false;
                }
                std::cout << strName << "=";
                std::cout.write(val, val_end - val);
                std::cout << std::endl;
                cout_url_eol(url);
            }
        }
        if (!ok) {
            std::cerr << "Error in line:\n" << line << std::endl;
            break;
        }
    }
    return ok;
}

bool AsciiEqualsIgnoreCase(const char* test, const char* lcase) {
    auto ascii_lc = [](char c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; };
    while (*test && ascii_lc(*test) == *lcase) test++, lcase++;
    return *test == 0 && *lcase == 0;
}

const char* end_of_file_name(const char* fname) {
    const char* last = fname + std::strlen(fname);
    for (const char* p = last; p > fname; ) {
        p--;
        switch (p[0]) {
        case '.':
            return p;
        case '/':
        case '\\':
            return last;
        }
    }
    return last;
}

void read_samples(const char* file_name) {
    const char* ext = end_of_file_name(file_name);
    if (!AsciiEqualsIgnoreCase(ext, ".json")) {
        std::string fn_out(file_name, ext);
        fn_out.append(".json");
        SamplesOutputJson out(std::move(fn_out));
        read_samples(file_name, out);
    } else {
        std::cerr << "Samples file can not be .json: " << file_name << std::endl;
    }
}

// Main

void test_interactive(const char* szBaseUrl);
void test_parser();
void test_setters();
void run_unit_tests();

int main(int argc, char *argv[])
{
    // set user-preferred locale
    setlocale(LC_ALL, "");

    if (argc > 1) {
        const char* flag = argv[1];
        if (flag[0] == '-') {
            switch (flag[1]) {
            case 'i':
                test_interactive(argc > 2 ? argv[2] : nullptr);
                return 0;
            case 'g':
                if (argc > 2) {
                    read_samples(argv[2]);
                    return 0;
                }
            case 't':
                if (argc > 2) {
                    SamplesOutput out;
                    read_samples(argv[2], out);
                    return 0;
                }
            }
        }
        std::cerr
            << "test_sample [-i [<base URL>]]\n"
            << "test_sample -g <samples file>\n"
            << "test_sample -t <samples file>"
            << std::endl;

    } else {
        test_parser();
        test_setters();
        run_unit_tests();
    }
    return 0;
}

void test_interactive(const char* szBaseUrl)
{
    std::cout << "Enter URL; enter empty line to exit\n";

    std::string str;
    while (std::getline(std::cin, str)) {
        if (str.empty()) break;
        url_testas(str.c_str(), szBaseUrl);
    }
}

void test_parser()
{
    url_testas("file://d:/laikina/%2e./tek%stas.txt", nullptr);
    url_testas("filesystem:http://www.example.com/temporary/", nullptr);

    url_testas("ssh://example.net");

    url_testas("blob:550e8400-e29b-41d4-a716-446655440000#aboutABBA");
    url_testas("invalid^scheme://example.com", nullptr);

    // iš; https://github.com/whatwg/url/issues/162
    url_testas("http://example.com/%61%62%63a%2e%64%65%7e%7f%80%81");

    url_testas("mailto:vardenis@example.com", nullptr);

#if !(defined(_MSC_VER) && _MSC_VER >= 1900)
    // MS VC 2015 and later bug: codecvt_utf8[_utf16] doesn't support char16_t, char32_t
    // https://connect.microsoft.com/VisualStudio/feedback/details/1348277/link-error-when-using-std-codecvt-utf8-utf16-char16-t
    // https://connect.microsoft.com/VisualStudio/feedback/details/1403302/unresolved-external-when-using-codecvt-utf8

    const char* szUrl = "http://user:pass@klausimėlis.lt/?key=ąče&uni=🙂#frag";
    url_testas(szUrl);
    // char16_t
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv16;
    url_testas(conv16.from_bytes(szUrl).c_str());
    // char32_t
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv32;
    url_testas(conv32.from_bytes(szUrl).c_str());
    url_testas(conv32.from_bytes("http://example.net").c_str());
    // wchar_t
    url_testas(to_wstr(szUrl).c_str());
    // --
#endif

    url_testas("http://user:pass@klausim%c4%97lis.lt/?key=ąče#frag");
    url_testas("http://user:pass@klausim%25lis.lt/?key=ąče#frag");

    url_testas("https://username:pass@word@example.com:123/path/data?abc=ąbč&key=value&key2=value2#fragid1-ą");

    url_testas("   wss\r:\n/\t/abc.lt/ \t ", nullptr);

    url_testas("file://example.com/bandymas/#123", nullptr);

    url_testas("http://example.com:8080/bandymas/#123", nullptr);
    url_testas("http://example.com:80/bandymas/?#", nullptr);

    // No need for null passwords
    // https://github.com/whatwg/url/issues/181
    url_testas("http://:@domain.lt/");
    // https://github.com/whatwg/url/pull/186
    url_testas("https://test:@test.lt/");

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
    url_testas("http://[1:2:3:4::6:7:8]/kelias/");  // rust-url bug (fixed)
    url_testas("http://[1:2:3:4:5:6:7:8]/kelias/");
    url_testas("http://[1:2::7:8]/kelias/");
    url_testas("http://[1:2:3::]/kelias/");
    url_testas("http://[::6:7:8]/kelias/");
    url_testas("http://[0::0]");
    url_testas("http://[::]");
    url_testas("http://[0:f:0:0:f:f:0:0]");
    url_testas("http://[::1.2.3.4]");
    // bounds checking
    url_testas("http://[::1.2.3.4.5]");
    url_testas("http://[1:2:3:4:5:6:1.2.3.4.5]");
    // https://github.com/whatwg/url/issues/195
    // URL standard bugs (see: "IPv6 parser" "10.7. If c is not the EOF code point, increase pointer by one.")
    // - praleis 'X' (ar jo vietoje bet kokį ne skaitmenį) be klaidų
    url_testas("http://[::1.2.3.4X]");
    // must be failure:
    url_testas("http://[::1.2.3.]");
    url_testas("http://[::1.2.]");
    url_testas("http://[::1.]");
    
    // jsdom/whatwg-url parser BUG (fixed: https://github.com/jsdom/whatwg-url/pull/66)
    // https://quuz.org/url/ IPv6 serializer bug (no compressing trailing zeros):
    url_testas("http://[2::0]");
    url_testas("http://[2::]");

    // port test
    // https://github.com/whatwg/url/issues/257#issuecomment-285553590
    url_testas("http://example.net:65535");
    url_testas("http://example.net:65536");
    url_testas("asdf://host:65535");
    url_testas("asdf://host:65536");

    // IDNA testai
    // http://www.unicode.org/reports/tr46/#Implementation_Notes
    url_testas("http://%E5%8D%81%zz.com/");
    url_testas("http://%C3%BF-abc.com/");

    // https://github.com/jsdom/whatwg-url/issues/50
    url_testas("https://r3---sn-p5qlsnz6.googlevideo.com/");

    // non "http://"
    url_testas("http:/example.net");
    url_testas("http:example.net");

    // test https://url.spec.whatwg.org/#path-state
    // 1.4.1. scheme is "file", url’s path is empty, and buffer is a Windows drive letter
    url_testas("file://example.net/C:/");
    url_testas("file://1.2.3.4/C:/");
    url_testas("file://[1::8]/C:/");

    // https://url.spec.whatwg.org/#concept-url-serializer
    url_testas("file:///example.net/C:/");
    url_testas("file:/example.net/C:/");
    url_testas("file:example.net/C:/");
    // C:
    url_testas("file://example.net/p/../C:/");
    url_testas("file://example.net/../C:/");
    // no warnings?
    url_testas("file:///C:/path");
    url_testas("file://C:/path");
    url_testas("file:/C:/path");
    url_testas("file:C:/path");

    url_testas("file:///nothost/path");
    url_testas("file://host/path");
    url_testas("file:/nothost/path");
    url_testas("file:nothost/path");

    // file and ? or #
    // jsdom/whatwg-url parser BUG
    url_testas("file:#frag");
    url_testas("file:?q=v");
    // papildomai
    url_testas("file:##frag");
    url_testas("file:??q=v");
    url_testas("file:#/pa/pa");
    url_testas("file:##/pa/pa");
    // only "file" scheme
    url_testas("file:");
    // kiti "file" atvejai
    url_testas("file:/");
    url_testas("file://");
    url_testas("file:///");

    // https://github.com/whatwg/url/issues/303
    url_testas("/c:/foo/bar", "file:///c:/baz/qux");
    url_testas("/test", "file:///c:/x");

    // https://github.com/whatwg/url/issues/304
    url_testas("C|", "file://host/dir/file");
    url_testas("C|#", "file://host/dir/file");
    url_testas("C|?", "file://host/dir/file");
    url_testas("C|/", "file://host/dir/file");
    url_testas("C|\\", "file://host/dir/file");
    url_testas("/C|", "file://host/dir/file");
    // papildomi mano testai
    url_testas("C", "file://host/dir/file");
    url_testas("C|a", "file://host/dir/file");

    // failure: empty host
    url_testas("http:#abc");

    // iš: https://github.com/whatwg/url/issues/97
    url_testas("file://y/.hostname = \"x:123\"");
    // https://github.com/whatwg/url/issues/210
    url_testas("file:///C%3A/a/b/c");
    url_testas("file:///C%7C/a/b/c");
    // mano išvesti
    url_testas("file:///c%3a/../b");
    url_testas("file:///c:/../b");
    // žr.: url_parser::parse_path(..): "d:" ne kelio pradžioje
    // turi persikelti į pradžią
    url_testas("file:///abc/../d:/../some.txt");
    // ar naršyklėse veiks (t.y. rodys failą):
    url_testas("file:///abc/../d:/some.txt");

    // UTF-8 percent encoded test
    url_testas("http://Ā©.com");
    url_testas("http://%C2%A9.com");
    url_testas("http://%C2©.com");
    url_testas("http://Ā%A9.com");
    url_testas("http://%C2%39.com");
    // https://github.com/whatwg/url/issues/215
    url_testas("http://example.com%A0");
    url_testas("http://%E2%98%83");

    // https://github.com/whatwg/url/issues/148
    url_testas("unknown://†/");
    url_testas("notspecial://H%4fSt/path");

    // Ignore repeated file slashes
    // https://github.com/whatwg/url/issues/232
    // https://github.com/whatwg/url/issues/232#issuecomment-278461743
    url_testas("file://localhost///foo/bar");
    url_testas("file://///foo/bar");
    // https://github.com/whatwg/url/issues/232#issuecomment-278717694
    url_testas("////////server/file", "file:///tmp/mock/path");
    url_testas("server/file", "file:///tmp/mock/path");
    // https://github.com/whatwg/url/issues/232#issuecomment-281263060
    url_testas("file://localhost///a//../..//");
    // https://github.com/whatwg/url/pull/278
    url_testas("/..//localhost//pig", "file://lion/");
    url_testas("file:/..//localhost//pig", "file://lion/");

    // domain to ASCII (VerifyDnsLength = false)
    url_testas("https://../");
    url_testas("https://x01234567890123456789012345678901234567890123456789012345678901†/");
    // https://url.spec.whatwg.org/#concept-ipv4-parser
    url_testas("https://0..0x300/");

    // https://github.com/w3c/web-platform-tests/pull/4504#issuecomment-270771165
    url_testas("https://\u001Fx");
    url_testas("https://xn--\u001Fx-");
}

void test_setters()
{
    whatwg::url url;
    url.parse("ws://example.org/foo/bar", nullptr);
    cout_url_eol(url);

    url.href("wss://%00/foo/bar"); // failure
    cout_url_eol(url);

    url.href("wss://example.org/foo/bar");
    cout_url_eol(url);

    url.protocol("http:");
    cout_url_eol(url);

    url.username("user01");
    url.password("pass@01");
    cout_url_eol(url);

    url.host("example.org:81");
    cout_url_eol(url);

    url.hostname("example.net");
    cout_url_eol(url);

    url.port("88");
    cout_url_eol(url);

    url.port("");
    cout_url_eol(url);

    url.pathname("/path");
    cout_url_eol(url);

    url.hash("#frag");
    cout_url_eol(url);

    url.search("?a=3");
    cout_url_eol(url);

    // test path
    url.pathname("/other/path");
    cout_url_eol(url);

    // test switch to file: scheme
    url.protocol("file:");
    cout_url_eol(url);

    url.hostname("localhost");
    cout_url_eol(url);

    // test windows drive letters and ..
    url.hostname("example.org");
    cout_url_eol(url);

    url.pathname("/c|/../path");
    cout_url_eol(url);

    // non-special
    url.parse("non-special:/path", nullptr);
    cout_url_eol(url);

    url.hostname("example.net");
    cout_url_eol(url);

    url.hostname("");
    cout_url_eol(url);

    // javscript: scheme test
    url.parse("JavaScript:alert(1)", nullptr);
    url.hash("#frag");
    cout_url_eol(url);
}

#include "buffer.h"

void run_unit_tests() {
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

    // IDNA sample
    whatwg::simple_buffer<char> buf8;
    whatwg::simple_buffer<char16_t> buf16;

    std::string str("xn--abc.com");
    whatwg::IDNToUnicode(str.data(), str.length(), buf8);
    std::wstring wstr(to_wstr(buf8.begin(), buf8.end()));
    whatwg::IDNToASCII((const char16_t*)wstr.data(), wstr.length(), buf16);
    std::wstring wstrR((const wchar_t*)buf16.data(), buf16.size());


    // port test
    whatwg::url url;
    url.parse("http://aaa/", nullptr);
    assert(url.port_int() == -1);
    assert(url.real_port_int() == 80);
    url.parse("http://aaa:8080/", nullptr);
    assert(url.port_int() == 8080);
    assert(url.real_port_int() == 8080);
    url.parse("non-special://aaa/", nullptr);
    assert(url.port_int() == -1);
    assert(url.real_port_int() == -1);
    url.parse("non-special://aaa:123/", nullptr);
    assert(url.port_int() == 123);
    assert(url.real_port_int() == 123);

    // IPv4 parser test
    const char* szEmpty = "";
    uint32_t ipv4 = 1;
    assert(whatwg::ipv4_parse_number(szEmpty, szEmpty, ipv4) == whatwg::url_result::Ok && ipv4 == 0);
    ipv4 = 1;
    assert(whatwg::ipv4_parse(szEmpty, szEmpty, ipv4) == whatwg::url_result::False);
}
