#ifndef WHATWG_URL_IDNA_H
#define WHATWG_URL_IDNA_H

#include "buffer.h"
#include "url_result.h"
#include <cstdint> // uint32_t, [char16_t, char32_t]

namespace whatwg {

url_result IDNToASCII(const char16_t* src, size_t src_len, simple_buffer<char16_t>& output);
url_result IDNToUnicode(const char* src, size_t src_len, simple_buffer<char>& output);

} // namespace whatwg

#endif // WHATWG_URL_IDNA_H
