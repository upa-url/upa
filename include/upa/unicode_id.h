// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#ifndef UPA_UNICODE_ID_H
#define UPA_UNICODE_ID_H

#include <cstddef>
#include <cstdint>

namespace upa::pattern::table {


// BEGIN-GENERATED
const std::uint8_t id_start_bit = 0x1;
const std::uint8_t id_part_bit = 0x10;
const std::size_t id_bit_shift = 2;
const char32_t id_bit_mask = 0x3;

const std::size_t id_block_shift = 5;
const std::uint32_t id_block_mask = 0x1F;

const char32_t id_start_default_start = 0x323B0;
const std::uint8_t id_start_default_value = 0;
const char32_t id_part_default_start = 0x323B0;
const std::uint8_t id_part_default_value = 0;
const char32_t id_part_spec_from = 0xE0100;
const char32_t id_part_spec_to = 0xE01EF;
const std::uint8_t id_part_spec_value = 0x1;

extern const std::uint8_t id_data[];
extern const std::uint8_t id_index[];
// END-GENERATED


inline bool in_table(std::uint8_t bit, char32_t cp) noexcept {
    const auto ind = cp >> id_bit_shift;
    const auto item = id_data[(id_index[ind >> id_block_shift] << id_block_shift) | (ind & id_block_mask)];
    return (item & (bit << (cp & id_bit_mask))) != 0;
}

inline bool is_identifier_start(char32_t cp) noexcept {
   if (cp >= id_start_default_start)
       return id_start_default_value;
   return in_table(id_start_bit, cp);
}

inline bool is_identifier_part(char32_t cp) noexcept {
    if (cp >= id_part_default_start) {
        if (cp >= id_part_spec_from && cp <= id_part_spec_to)
            return id_part_spec_value;
        return id_part_default_value;
    }
    return in_table(id_part_bit, cp);
}


} // namespace upa::pattern::table

#endif // UPA_UNICODE_ID_H
