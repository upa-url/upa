#include "url_ip.h"

namespace whatwg {

template <typename UIntT>
inline void unsigned_to_str(UIntT num, std::string& output, UIntT base) {
    static const char digit[] = "0123456789abcdef";

    // divider
    UIntT divider = 1;
    const UIntT num0 = num / base;
    while (divider <= num0) divider *= base;

    // convert
    do {
        output.push_back(digit[num / divider % base]);
        divider /= base;
    } while (divider);
}

// IPv4

void ipv4_serialize(uint32_t ipv4, std::string& output) {
    for (unsigned shift = 24; shift != 0; shift -= 8) {
        unsigned_to_str<uint32_t>((ipv4 >> shift) & 0xFF, output, 10);
        output.push_back('.');
    }
    unsigned_to_str<uint32_t>(ipv4 & 0xFF, output, 10);
}

// IPv6

static int longest_zero_sequence(const uint16_t* first, const uint16_t* last, const uint16_t*& start, const uint16_t*& end) {
    int last_count = 0;
    for (auto it = first; it != last; it++) {
        if (*it == 0) {
            auto ite = it + 1;
            while (ite != last && *ite == 0) ite++;
            const int count = ite - it;
            if (last_count < count) {
                last_count = count;
                start = it;
                end = ite;
            }
            if (ite == last) break;
            it = ite; // it++ in loop skips not 0
        }
    }
    return last_count;
}

void ipv6_serialize(const uint16_t(&pieces)[8], std::string& output) {
    const auto first = std::begin(pieces);
    const auto last = std::end(pieces);

    const uint16_t *zero_start;
    const uint16_t *zero_end;
    if (longest_zero_sequence(first, last, zero_start, zero_end) <= 1)
        zero_start = zero_end = nullptr;
    
    bool need_colon = false;
    for (auto it = first; it != last; ) {
        if (it == zero_start) {
            need_colon = false;
            output.append("::");
            it = zero_end;
        } else {
            if (need_colon) output.push_back(':');
            else need_colon = true;
            unsigned_to_str<uint32_t>(*it, output, 16);
            it++;
        }
    }
}


} // namespace whatwg
