// Copyright 2023-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_UNICODE_ID_H
#define UPA_UNICODE_ID_H

#include <cstddef>
#include <cstdint>

namespace upa::pattern::table {


// BEGIN-GENERATED
const std::uint8_t bit_of_id_start = 0x1;
const std::uint8_t bit_of_id_part = 0x10;
const std::size_t bit_shift = 2;
const char32_t bit_mask = 0x3;

const std::size_t blockShift = 5;
const std::uint32_t blockMask = 0x1F;

const char32_t default_start_of_id_start = 0x323B0;
const std::uint8_t default_value_of_id_start = 0;
const char32_t default_start_of_id_part = 0x323B0;
const std::uint8_t default_value_of_id_part = 0;
const char32_t spec_from_of_id_part = 0xE0100;
const char32_t spec_to_of_id_part = 0xE01EF;
const std::uint8_t spec_value_of_id_part = 0x1;

extern std::uint8_t blockData[];
extern std::uint8_t blockIndex[];
// END-GENERATED


inline bool in_table(std::uint8_t bit, char32_t cp) noexcept {
    const auto ind = cp >> bit_shift;
    const auto item = blockData[(blockIndex[ind >> blockShift] << blockShift) | (ind & blockMask)];
    return (item & (bit << (cp & bit_mask))) != 0;
}

inline bool is_identifier_start(char32_t cp) noexcept {
   if (cp >= default_start_of_id_start)
       return default_value_of_id_start;
   return in_table(bit_of_id_start, cp);
}

inline bool is_identifier_part(char32_t cp) noexcept {
    if (cp >= default_start_of_id_part) {
        if (cp >= spec_from_of_id_part && cp <= spec_to_of_id_part)
            return spec_value_of_id_part;
        return default_value_of_id_part;
    }
    return in_table(bit_of_id_part, cp);
}


} // namespace upa::pattern::table

#endif // UPA_UNICODE_ID_H
