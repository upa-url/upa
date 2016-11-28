#ifndef WHATWG_URLCANON_H
#define WHATWG_URLCANON_H

#include "url_util.h"
#include <string>

namespace whatwg {
namespace detail {

// https://cs.chromium.org/chromium/src/url/url_canon_internal.h

// Character type handling -----------------------------------------------------

// Bits that identify different character types. These types identify different
// bits that are set for each 8-bit character in the kSharedCharTypeTable.
enum SharedCharTypes {
  // Characters that do not require escaping in queries. Characters that do
  // not have this flag will be escaped; see url_canon_query.cc
  CHAR_QUERY = 1,

  // Valid in the username/password field.
  CHAR_USERINFO = 2,

  // Valid in a IPv4 address (digits plus dot and 'x' for hex).
  CHAR_IPV4 = 4,

  // Valid in an ASCII-representation of a hex digit (as in %-escaped).
  CHAR_HEX = 8,

  // Valid in an ASCII-representation of a decimal digit.
  CHAR_DEC = 16,

  // Valid in an ASCII-representation of an octal digit.
  CHAR_OCT = 32,

  // Characters that do not require escaping in encodeURIComponent. Characters
  // that do not have this flag will be escaped; see url_util.cc.
  CHAR_COMPONENT = 64,

  // Characters that do not require escaping in path (not default encode set)
  CHAR_DEFAULT = 128,
};

// This table contains the flags in SharedCharTypes for each 8-bit character.
// Some canonicalization functions have their own specialized lookup table.
// For those with simple requirements, we have collected the flags in one
// place so there are fewer lookup tables to load into the CPU cache.
//
// Using an unsigned char type has a small but measurable performance benefit
// over using a 32-bit number.
extern const unsigned char kSharedCharTypeTable[0x100];

// More readable wrappers around the character type lookup table.
inline bool IsCharOfType(unsigned char c, SharedCharTypes type) {
  return !!(kSharedCharTypeTable[c] & type);
}
inline bool IsQueryChar(unsigned char c) {
  return IsCharOfType(c, CHAR_QUERY);
}
inline bool IsIPv4Char(unsigned char c) {
  return IsCharOfType(c, CHAR_IPV4);
}
inline bool IsHexChar(unsigned char c) {
  return IsCharOfType(c, CHAR_HEX);
}
inline bool IsComponentChar(unsigned char c) {
  return IsCharOfType(c, CHAR_COMPONENT);
}

// Appends the given string to the output, escaping characters that do not
// match the given |type| in SharedCharTypes.
void AppendStringOfType(const char* first, const char* last,
    SharedCharTypes type, std::string& output);
void AppendStringOfType(const char16_t* first, const char16_t* last,
    SharedCharTypes type, std::string& output);
void AppendStringOfType(const char32_t* first, const char32_t* last,
    SharedCharTypes type, std::string& output);

// Maps the hex numerical values 0x0 to 0xf to the corresponding ASCII digit
// that will be used to represent it.
extern const char kHexCharLookup[0x10];

// This lookup table allows fast conversion between ASCII hex letters and their
// corresponding numerical value. The 8-bit range is divided up into 8
// regions of 0x20 characters each. Each of the three character types (numbers,
// uppercase, lowercase) falls into different regions of this range. The table
// contains the amount to subtract from characters in that range to get at
// the corresponding numerical value.
//
// See HexDigitToValue for the lookup.
extern const char kCharToHexLookup[8];

// Assumes the input is a valid hex digit! Call IsHexChar before using this.
inline unsigned char HexCharToValue(unsigned char c) {
  return c - kCharToHexLookup[c / 0x20];
}

// Indicates if the given character is a dot or dot equivalent, returning the
// number of characters taken by it. This will be one for a literal dot, 3 for
// an escaped dot. If the character is not a dot, this will return 0.
template<typename CHAR>
inline int IsDot(const CHAR* spec, int offset, int end) {
  if (spec[offset] == '.') {
    return 1;
  } else if (spec[offset] == '%' && offset + 3 <= end &&
             spec[offset + 1] == '2' &&
             (spec[offset + 2] == 'e' || spec[offset + 2] == 'E')) {
    // Found "%2e"
    return 3;
  }
  return 0;
}

// Write a single character, escaped, to the output. This always escapes: it
// does no checking that thee character requires escaping.
// Escaping makes sense only 8 bit chars, so code works in all cases of
// input parameters (8/16bit).
template<typename UINCHAR, typename OUTCHAR>
inline void AppendEscapedChar(UINCHAR ch, std::basic_string<OUTCHAR>& output) {
  output.push_back('%');
  output.push_back(kHexCharLookup[(ch >> 4) & 0xf]);
  output.push_back(kHexCharLookup[ch & 0xf]);
}

// The character we'll substitute for undecodable or invalid characters.
//extern const base::char16 kUnicodeReplacementCharacter;

// UTF-8 functions ------------------------------------------------------------

// Generic To-UTF-8 converter. This will call the given append method for each
// character that should be appended, with the given output method. Wrappers
// are provided below for escaped and non-escaped versions of this.
//
// The char_value must have already been checked that it's a valid Unicode
// character.
template<class Output, void Appender(unsigned char, Output&)>
inline void DoAppendUTF8(unsigned char_value, Output& output) {
    if (char_value <= 0x7f) {
        Appender(static_cast<unsigned char>(char_value), output);
    } else if (char_value <= 0x7ff) {
        // 110xxxxx 10xxxxxx
        Appender(static_cast<unsigned char>(0xC0 | (char_value >> 6)),
            output);
        Appender(static_cast<unsigned char>(0x80 | (char_value & 0x3f)),
            output);
    } else if (char_value <= 0xffff) {
        // 1110xxxx 10xxxxxx 10xxxxxx
        Appender(static_cast<unsigned char>(0xe0 | (char_value >> 12)),
            output);
        Appender(static_cast<unsigned char>(0x80 | ((char_value >> 6) & 0x3f)),
            output);
        Appender(static_cast<unsigned char>(0x80 | (char_value & 0x3f)),
            output);
    } else if (char_value <= 0x10FFFF) {  // Max Unicode code point.
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        Appender(static_cast<unsigned char>(0xf0 | (char_value >> 18)),
            output);
        Appender(static_cast<unsigned char>(0x80 | ((char_value >> 12) & 0x3f)),
            output);
        Appender(static_cast<unsigned char>(0x80 | ((char_value >> 6) & 0x3f)),
            output);
        Appender(static_cast<unsigned char>(0x80 | (char_value & 0x3f)),
            output);
    } else {
        // Invalid UTF-8 character (>20 bits).
        //TODO: NOTREACHED();
    }
}

// Helper used by AppendUTF8Value below. We use an unsigned parameter so there
// are no funny sign problems with the input, but then have to convert it to
// a regular char for appending.
inline void AppendCharToOutput(unsigned char ch, std::string& output) {
    output.push_back(static_cast<char>(ch));
}

// Writes the given character to the output as UTF-8. This does NO checking
// of the validity of the Unicode characters; the caller should ensure that
// the value it is appending is valid to append.
inline void AppendUTF8Value(unsigned char_value, std::string& output) {
    DoAppendUTF8<std::string, AppendCharToOutput>(char_value, output);
}

// Writes the given character to the output as UTF-8, escaping ALL
// characters (even when they are ASCII). This does NO checking of the
// validity of the Unicode characters; the caller should ensure that the value
// it is appending is valid to append.
inline void AppendUTF8EscapedValue(unsigned char_value, std::string& output) {
    DoAppendUTF8<std::string, AppendEscapedChar>(char_value, output);
}

#if 0
// UTF-16 functions -----------------------------------------------------------

// Equivalent to U16_APPEND_UNSAFE in ICU but uses our output method.
inline void AppendUTF16Value(unsigned code_point, std::basic_string<char16_t>& output) {
    if (code_point > 0xffff) {
        output.push_back(static_cast<char16_t>((code_point >> 10) + 0xd7c0));
        output.push_back(static_cast<char16_t>((code_point & 0x3ff) | 0xdc00));
    } else {
        output.push_back(static_cast<char16_t>(code_point));
    }
}
#endif

// Escaping functions ---------------------------------------------------------

// Writes the given character to the output as UTF-8, escaped. Call this
// function only when the input is wide. Returns true on success. Failure
// means there was some problem with the encoding, we'll still try to
// update the |*begin| pointer and add a placeholder character to the
// output so processing can continue.

template <typename CharT>
inline bool AppendUTF8EscapedChar(const CharT*& first, const CharT* last, std::string& output) {
    // url_util::read_utf_char(..) will handle invalid characters for us and give
    // us the kUnicodeReplacementCharacter, so we don't have to do special
    // checking after failure, just pass through the failure to the caller.
    uint32_t code_point;
    bool success = url_util::read_utf_char(first, last, code_point);
    AppendUTF8EscapedValue(code_point, output);
    return success;
}


} // namespace detail
} // namespace whatwg

#endif // WHATWG_URLCANON_H
