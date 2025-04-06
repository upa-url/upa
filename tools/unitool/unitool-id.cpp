// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "unicode_data_tools.h"
#include <filesystem>

using namespace upa::tools;

constexpr std::size_t align_size(std::size_t alignment, std::size_t size) noexcept {
    return ((size + (alignment - 1)) / alignment)* alignment;
}


void make_unicode_id_table(const std::filesystem::path& data_path) {
    using item_type = uint8_t;
    using item_num_type = item_type;

    const int index_levels = 1; // 1 arba 2
    const item_num_type bit_of_id_start = 0x01;
    const item_num_type bit_of_id_part = 0x10;
    const std::size_t bit_shift = 2;
    const char32_t bit_mask = 3;

    std::vector<item_type> code_points_of_id_start_ch(MAX_CODE_POINT + 1);
    std::vector<item_type> code_points_of_id_part_ch(MAX_CODE_POINT + 1);

    auto file_name = data_path / "DerivedCoreProperties.txt";
    parse_UnicodeData<1>(file_name, [&](int cp0, int cp1, const auto& col) {
        // [C C C C S S S S]
        if (col[0] == "ID_Start") {
            for (int cp = cp0; cp <= cp1; cp++)
                code_points_of_id_start_ch[cp] = 1;
        }
        else if (col[0] == "ID_Continue") {
            for (int cp = cp0; cp <= cp1; cp++)
                code_points_of_id_part_ch[cp] = 1;
        }
    });

    // ID_Start: https://tc39.es/ecma262/#prod-IdentifierStartChar
    code_points_of_id_start_ch['$'] = 1;
    code_points_of_id_start_ch['_'] = 1;
    // ID_Continue: https://tc39.es/ecma262/#prod-IdentifierPartChar
    code_points_of_id_part_ch['$'] = 1;
    code_points_of_id_part_ch[0x200C] = 1; // <ZWNJ>
    code_points_of_id_part_ch[0x200D] = 1; // <ZWJ>

    // For new unicode standard version (>15.0) revise max_range_count values
    special_ranges<item_num_type> spec_id_start_ch(code_points_of_id_start_ch, 1);
    special_ranges<item_num_type> spec_id_part_ch(code_points_of_id_part_ch, 2);

    // align on divider
    const std::size_t code_point_count = align_size(4, std::max(
        spec_id_start_ch.m_range[0].from,
        spec_id_part_ch.m_range[0].from));

    // prepare bitset array
    const std::size_t count = code_point_count >> 2; // divide by 4
    std::vector<item_type> all_data(count);

    constexpr auto add_code_point = [](std::vector<item_type>& arr, uint8_t bit, uint32_t cp) {
        arr[cp >> 2] |= (bit << (cp & 3));
    };
    for (std::uint32_t cp = 0; cp < code_point_count; ++cp) {
        if (code_points_of_id_start_ch[cp])
            add_code_point(all_data, bit_of_id_start, cp);
        if (code_points_of_id_part_ch[cp])
            add_code_point(all_data, bit_of_id_part, cp);
    }

    // Find block size
    const auto binf = find_block_size(all_data, count, sizeof(item_num_type), index_levels);
    const size_t block_size = binf.block_size;

    // memory used
    std::cout << "block_size=" << block_size << "; mem=" << binf.total_mem() << "\n";

    //=======================================================================
    // Generate code

    const char* sz_item_num_type = getUIntType<item_num_type>();

    file_name = data_path / "GEN-unicode_id-tables.txt";
    std::ofstream fout(file_name, std::ios_base::out);
    if (!fout.is_open()) {
        std::cerr << "Can't open destination file: " << file_name << std::endl;
        return;
    }

    file_name = data_path / "GEN-unicode_id-tables.H.txt";
    std::ofstream fout_head(file_name, std::ios_base::out);
    if (!fout_head.is_open()) {
        std::cerr << "Can't open destination file: " << file_name << std::endl;
        return;
    }
    // Constants
    output_unsigned_constant(fout_head, "bit_of_id_start", bit_of_id_start, 16);
    output_unsigned_constant(fout_head, "bit_of_id_part", bit_of_id_part, 16);
    output_unsigned_constant(fout_head, "std::size_t", "bit_shift", bit_shift, 10);
    output_unsigned_constant(fout_head, "char32_t", "bit_mask", bit_mask, 16);
    fout_head << "\n";
    output_unsigned_constant(fout_head, "std::size_t", "blockShift", binf.size_shift, 10);
    output_unsigned_constant(fout_head, "blockMask", binf.code_point_mask(), 16);
    fout_head << "\n";
    // IdentifierStartChar
    output_unsigned_constant(fout_head, "char32_t", "default_start_of_id_start", spec_id_start_ch.m_range[0].from, 16);
    output_unsigned_constant(fout_head, sz_item_num_type, "default_value_of_id_start", spec_id_start_ch.m_range[0].value, 16);
    // IdentifierPartChar
    output_unsigned_constant(fout_head, "char32_t", "default_start_of_id_part", spec_id_part_ch.m_range[0].from, 16);
    output_unsigned_constant(fout_head, sz_item_num_type, "default_value_of_id_part", spec_id_part_ch.m_range[0].value, 16);
    if (spec_id_part_ch.m_range.size() >= 2) {
        output_unsigned_constant(fout_head, "char32_t", "spec_from_of_id_part", spec_id_part_ch.m_range[1].from, 16);
        output_unsigned_constant(fout_head, "char32_t", "spec_to_of_id_part", spec_id_part_ch.m_range[1].to, 16);
        output_unsigned_constant(fout_head, sz_item_num_type, "spec_value_of_id_part", spec_id_part_ch.m_range[1].value, 16);
    }
    fout_head << "\n";
    // ---

    std::vector<int> blockIndex;

    fout_head << "extern " << sz_item_num_type << " blockData[];\n";
    fout << sz_item_num_type << " blockData[] = {";
    {
        OutputFmt outfmt(fout, 100);

        typedef std::map<array_view<item_type>, int> BlokcsMap;
        BlokcsMap blocks;
        int index = 0;
        for (size_t ind = 0; ind < count; ind += block_size) {
            size_t chunk_size = std::min(block_size, all_data.size() - ind);
            array_view<item_type> block(all_data.data() + ind, chunk_size);

            auto res = blocks.insert(BlokcsMap::value_type(block, index));
            if (res.second) {
                for (const auto& item : block) {
                    outfmt.output(static_cast<item_num_type>(item), 16);
                }
                blockIndex.push_back(index);
                index++;
            } else {
                // index of previously inserted block
                blockIndex.push_back(res.first->second);
            }
        }
    }
    fout << "};\n\n";

    if (index_levels == 1) {
        // Vieno lygio indeksas
        const char* sztype = getUIntType(blockIndex);
        fout_head << "extern " << sztype << " blockIndex[];\n";
        fout << sztype << " blockIndex[] = {";
        {
            OutputFmt outfmt(fout, 100);
            for (int index : blockIndex) {
                outfmt.output(index, 10);
            }
        }
        fout << "};\n\n";
    }

    if (index_levels == 2) {
        // Dviejų lygių indeksas
        std::vector<int> indexToIndex;
        const char* sztype = getUIntType(blockIndex);
        fout_head << "extern " << sztype << " indexToBlock[];\n";
        fout << sztype << " indexToBlock[] = {";
        {
            size_t count = blockIndex.size();
            std::cout << "=== Index BLOCK ===\n";
            block_info bi = find_block_size(blockIndex, count, getUIntSize(blockIndex));

            OutputFmt outfmt(fout, 100);

            typedef std::map<array_view<int>, int> BlokcsMap;
            BlokcsMap blocks;
            int index = 0;
            for (size_t ind = 0; ind < count; ind += bi.block_size) {
                size_t chunk_size = std::min(bi.block_size, count - ind);
                array_view<int> block(blockIndex.data() + ind, chunk_size);

                auto res = blocks.insert(BlokcsMap::value_type(block, index));
                if (res.second) {
                    for (const int value : block) {
                        outfmt.output(value, 10);
                    }
                    indexToIndex.push_back(index);
                    index++;
                } else {
                    // index of previously inserted block
                    indexToIndex.push_back(res.first->second);
                }
            }
        }
        fout << "};\n\n";
        sztype = getUIntType(indexToIndex);
        fout_head << "extern " << sztype << " indexToIndex[];\n";
        fout << sztype << " indexToIndex[] = {";
        {
            OutputFmt outfmt(fout, 100);
            for (int ind : indexToIndex) {
                outfmt.output(ind, 10);
            }
        }
        fout << "};\n\n";
    }
}

int main(int argc, char* argv[]) {

    if (argc > 1)
        make_unicode_id_table(argv[1]);
    else
        std::cout << "unicode_id <directory of DerivedCoreProperties.txt file>\n";
}
