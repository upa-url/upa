#ifndef WHATWG_URL_IDNA_H
#define WHATWG_URL_IDNA_H

#include <cstdint> // uint32_t, [char16_t, char32_t]
#include <vector>

namespace whatwg {

bool IDNToASCII(const char16_t* src, int src_len, std::vector<char16_t>& output);

} // namespace whatwg

#endif // WHATWG_URL_IDNA_H
