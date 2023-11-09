// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// https://github.com/kazuho/picojson
#include "picojson/picojson.h"

#include <fstream>
#include <iostream>
#include <iterator>

// PicoJSON HACK
//
// Add _parse_string(...) specialization, which replaces unpaired surrogates
// with 'REPLACEMENT CHARACTER' (U+FFFD).

namespace detail {

inline void append_utf8(std::string& out, int uni_ch) {
    // to utf-8
    if (uni_ch < 0x80) {
        out.push_back(static_cast<char>(uni_ch));
    } else {
        if (uni_ch < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (uni_ch >> 6)));
        } else {
            if (uni_ch < 0x10000) {
                out.push_back(static_cast<char>(0xe0 | (uni_ch >> 12)));
            } else {
                out.push_back(static_cast<char>(0xf0 | (uni_ch >> 18)));
                out.push_back(static_cast<char>(0x80 | ((uni_ch >> 12) & 0x3f)));
            }
            out.push_back(static_cast<char>(0x80 | ((uni_ch >> 6) & 0x3f)));
        }
        out.push_back(static_cast<char>(0x80 | (uni_ch & 0x3f)));
    }
}

} // namespace detail

namespace picojson {

template <typename Iter>
inline bool _parse_string(std::string &out, input<Iter> &in) {
    while (1) {
        int ch = in.getc();
        if (ch < ' ') {
            in.ungetc();
            return false;
        } else if (ch == '"') {
            return true;
        } else if (ch == '\\') {
            bool next_escape;
            do {
                next_escape = false;
                if ((ch = in.getc()) == -1) {
                    return false;
                }
                switch (ch) {
#define MAP(sym, val)                  \
            case sym:                  \
                out.push_back(val);    \
                break
                    MAP('"', '\"');
                    MAP('\\', '\\');
                    MAP('/', '/');
                    MAP('b', '\b');
                    MAP('f', '\f');
                    MAP('n', '\n');
                    MAP('r', '\r');
                    MAP('t', '\t');
#undef MAP
                case 'u': {
                    int uni_ch;
                    if ((uni_ch = picojson::_parse_quadhex(in)) == -1) {
                        return false;
                    }
                    while (0xd800 <= uni_ch && uni_ch <= 0xdfff) {
                        if (0xdc00 <= uni_ch) {
                            // a second 16-bit of a surrogate pair appeared
                            uni_ch = 0xFFFD;
                            break;
                        }
                        // first 16-bit of surrogate pair, get the next one
                        if (in.getc() != '\\') {
                            in.ungetc();
                            uni_ch = 0xFFFD;
                            break;
                        }
                        if (in.getc() != 'u') {
                            in.ungetc();
                            uni_ch = 0xFFFD;
                            next_escape = true;
                            break;
                        }
                        int second = picojson::_parse_quadhex(in);
                        if (!(0xdc00 <= second && second <= 0xdfff)) {
                            detail::append_utf8(out, 0xFFFD);
                            uni_ch = second;
                            continue;
                        }
                        // surrogates pair
                        uni_ch = ((uni_ch - 0xd800) << 10) | ((second - 0xdc00) & 0x3ff);
                        uni_ch += 0x10000;
                        break;
                    }
                    detail::append_utf8(out, uni_ch);
                    break;
                }
                default:
                    return false;
                }
            } while (next_escape);
        } else {
            out.push_back(static_cast<char>(ch));
        }
    }
    return false;
}

} // namespace picojson


namespace json_util {

enum {
    ERR_OK = 0,
    ERR_OPEN = 2,
    ERR_JSON = 4,
    ERR_EXCEPTION = 8
};

// Parses a JSON file using the specified PicoJSON parsing context

template <typename Context>
inline int load_file(Context& ctx, const char* file_name, const char* title = nullptr) {
    try {
        if (title)
            std::cout << title << ": " << file_name << '\n';
        else
            std::cout << "========== " << file_name << " ==========\n";

        std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
        if (!file.is_open()) {
            std::cerr << "Can't open file: " << file_name << std::endl;
            return ERR_OPEN;
        }

        std::string err;

        // for unformatted reading use std::istreambuf_iterator
        // http://stackoverflow.com/a/17776228/3908097
        picojson::_parse(ctx, std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), &err);

        if (!err.empty()) {
            std::cerr << err << std::endl;
            return ERR_JSON; // JSON error
        }
    }
    catch (std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return ERR_EXCEPTION;
    }
    return ERR_OK;
}

// Parsing context to read root array and invoke callback on each item

template <class OnArrayItem>
class root_array_context : public picojson::deny_parse_context {
    OnArrayItem on_array_item_;
public:
    root_array_context(OnArrayItem on_array_item)
        : on_array_item_(on_array_item)
    {}

    // array as root
    bool parse_array_start() { return true; }
    bool parse_array_stop(std::size_t) { return true; }

    template <typename Iter>
    bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
        picojson::value item;

        // parse the array item
        picojson::default_parse_context ctx(&item);
        if (!picojson::_parse(ctx, in))
            return false;

        // callback with array item
        return on_array_item_(item);
    }

    // deny object as root
    bool parse_object_start() { return false; }
    bool parse_object_stop() { return false; }
};

// Parsing context to read root object whose values are arrays,
// and invoke callback on each array item

template <class OnArrayItem, class OnKeyFilter>
class object_array_context : public picojson::deny_parse_context {
    OnArrayItem m_on_array_item;
    OnKeyFilter m_on_key_filter;
    int m_level = 1;
    std::string m_name;
public:
    object_array_context(OnArrayItem on_array_item, OnKeyFilter
            on_key_filter = [](const std::string&) { return true; })
        : m_on_array_item(on_array_item)
        , m_on_key_filter(on_key_filter)
    {}

    // array
    bool parse_array_start() { return m_level == 2; }
    bool parse_array_stop(std::size_t) { return m_level == 2; }

    template <typename Iter>
    bool parse_array_item(picojson::input<Iter>& in, std::size_t) {
        picojson::value item;

        // parse the array item
        picojson::default_parse_context ctx(&item);
        if (!picojson::_parse(ctx, in))
            return false;

        // callback with array item
        return m_on_array_item(m_name, item);
    }

    // object only as root
    bool parse_object_start() { return m_level == 1; }
    bool parse_object_stop() { return m_level == 1; }

    template <typename Iter>
    bool parse_object_item(picojson::input<Iter>& in, const std::string& name) {
        if (!m_on_key_filter(name)) {
            // skip
            picojson::null_parse_context nullctx;
            return picojson::_parse(nullctx, in);
        }
        m_name = name;
        // parse array
        ++m_level;
        const bool res = picojson::_parse(*this, in);
        --m_level;
        return res;
    }
};


}
