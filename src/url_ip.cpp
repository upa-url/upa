// Copyright 2016-2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/url_ip.h"
#include "upa/util.h"

namespace upa {

// IPv4 serializer
// https://url.spec.whatwg.org/#concept-ipv4-serializer

void ipv4_serialize(std::uint32_t ipv4, std::string& output) {
    for (unsigned shift = 24; shift != 0; shift -= 8) {
        util::unsigned_to_str<std::uint32_t>((ipv4 >> shift) & 0xFF, output, 10);
        output.push_back('.');
    }
    util::unsigned_to_str<std::uint32_t>(ipv4 & 0xFF, output, 10);
}

// IPv6 serializer
// https://url.spec.whatwg.org/#concept-ipv6-serializer

namespace {

inline std::size_t longest_zero_sequence(
    const std::uint16_t* first, const std::uint16_t* last,
    const std::uint16_t*& compress)
{
    // The sequence to compress should be longer than 1 zero
    std::size_t last_count = 1;
    for (auto it = first; it != last; ++it) {
        if (*it == 0) {
            auto ite = it + 1;
            while (ite != last && *ite == 0) ++ite;
            const std::size_t count = ite - it;
            if (last_count < count) {
                last_count = count;
                compress = it;
            }
            if (ite == last) break;
            it = ite; // ++it in the loop skips non-zero number
        }
    }
    return last_count;
}

} // namespace

void ipv6_serialize(const std::uint16_t(&address)[8], std::string& output) {
    const std::uint16_t *first = std::begin(address);
    const std::uint16_t *last = std::end(address);

    const std::uint16_t *compress = nullptr;
    const auto compress_length = longest_zero_sequence(first, last, compress);

    // "it" pointer corresponds to pieceIndex in the URL standard
    for (auto it = first; true;) {
        if (it == compress) {
            output.append("::", it == first ? 2 : 1);
            it += compress_length;
            if (it == last) break;
        }
        util::unsigned_to_str<std::uint32_t>(*it, output, 16);
        if (++it == last) break;
        output.push_back(':');
    }
}


} // namespace upa
