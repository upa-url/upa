#ifndef WHATWG_URL_IDNA_H
#define WHATWG_URL_IDNA_H

#include "buffer.h"
#include <cstdint> // uint32_t, [char16_t, char32_t]

namespace whatwg {

bool IDNToASCII(const char16_t* src, int src_len, simple_buffer<char16_t>& output);

} // namespace whatwg

#endif // WHATWG_URL_IDNA_H
