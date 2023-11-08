// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

// https://github.com/kazuho/picojson
#include "picojson/picojson.h"

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
