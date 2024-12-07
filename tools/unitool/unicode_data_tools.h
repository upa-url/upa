// Copyright 2017-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace upa {
namespace tools {

// Split

template<class InputIt, class T, class FunT>
inline void split(InputIt first, InputIt last, const T& delim, FunT output) {
    auto start = first;
    for (auto it = first; it != last;) {
        if (*it == delim) {
            output(start, it);
            start = ++it;
        } else {
            it++;
        }
    }
    output(start, last);
}

// Utility

inline void AsciiTrimSpaceTabs(const char*& first, const char*& last) {
    auto ascii_ws = [](char c) { return c == ' ' || c == '\t'; };
    // trim space
    while (first < last && ascii_ws(*first)) first++;
    while (first < last && ascii_ws(*(last - 1))) last--;
}

inline std::string get_column(const std::string& line, std::size_t& pos) {
    // Columns (c1, c2,...) are separated by semicolons
    std::size_t pos_end = line.find(';', pos);
    if (pos_end == line.npos) pos_end = line.length();

    // Leading and trailing spaces and tabs in each column are ignored
    const char* first = line.data() + pos;
    const char* last = line.data() + pos_end;
    AsciiTrimSpaceTabs(first, last);

    // skip ';'
    pos = pos_end < line.length() ? pos_end + 1 : pos_end;
    return std::string(first, last);
}

// string <--> number

template <typename UIntT>
inline void unsigned_to_str(UIntT num, std::string& output, UIntT base) {
    static const char digit[] = "0123456789ABCDEF";

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

template <typename UIntT>
inline void unsigned_to_numstr(UIntT num, std::string& output, int base) {
    if (num > 0) {
        switch (base) {
        case 8: output += '0'; break;
        case 16: output += "0x"; break;
        }
    }
    unsigned_to_str(num, output, static_cast<UIntT>(base));
}

template <typename UIntT>
inline std::string unsigned_to_numstr(UIntT num, int base) {
    std::string strNum;
    unsigned_to_numstr<UIntT>(num, strNum, base);
    return strNum;
}

inline static int hexstr_to_int(const char* first, const char* last) {
    if (first == last)
        throw std::runtime_error("invalid hex number");

    int num = 0;
    for (auto it = first; it != last; it++) {
        char c = *it;
        int digit;

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else
            throw std::runtime_error("invalid hex number");
        num = num * 0x10 + digit;
    }
    return num;
}

// Char type name

template <class T = char>
inline const char* getCharType(size_t item_size = sizeof(T)) {
    switch (item_size)
    {
    case 1: return "char";
    case 2: return "char16_t";
    case 4: return "char32_t";
    default: return "???";
    }
}

// Integer type name

template <class T>
inline std::size_t getUIntSize(const std::vector<T>& arr, size_t item_size = sizeof(T)) {
    size_t max_size = 0;
    for (const T& v : arr) {
        const size_t size = (v <= 0xFF ? 1 : (v <= 0xFFFF ? 2 : (v <= 0xFFFFFFFF ? 4 : 8)));
        if (max_size < size) max_size = size;
        if (size == item_size) break;
    }
    return max_size;
}

template <class T = std::uint32_t>
inline const char* getUIntType(size_t item_size = sizeof(T)) {
    switch (item_size)
    {
    case 1: return "std::uint8_t";
    case 2: return "std::uint16_t";
    case 4: return "std::uint32_t";
    case 8: return "std::uint64_t";
    default: return "???";
    }
}

template <class T>
inline const char* getUIntType(const std::vector<T>& arr, size_t item_size = sizeof(T)) {
    return getUIntType(getUIntSize(arr, item_size));
}

template <typename T>
inline void output_unsigned_constant(std::ostream& out, const char* type, const char* name, T value, int base) {
    out << "const " << type << ' ' << name << " = " << unsigned_to_numstr(value, base) << ";\n";
}

template <typename T>
inline void output_unsigned_constant(std::ostream& out, const char* name, T value, int base) {
    output_unsigned_constant(out, getUIntType<T>(), name, value, base);
}

// Parse input file

template <int cols_count, class OutputFun>
inline void parse_UnicodeData(const std::filesystem::path& file_name, OutputFun outputFun)
{
    std::cout << "FILE: " << file_name << std::endl;
    std::ifstream file(file_name, std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Can't open data file: " << file_name << std::endl;
        return;
    }

    // parse lines
    int line_num = 0;
    std::string line;
    while (std::getline(file, line)) {
        line_num++;
        // Comments are indicated with hash marks
        auto i_comment = line.find('#');
        if (i_comment != line.npos)
            line.resize(i_comment);
        // got line without comment
        if (line.length() > 0) {
            try {
                std::size_t pos = 0;
                const std::string cpstr = get_column(line, pos);
                std::array<std::string, cols_count> col;
                for (int i = 0; i < cols_count; ++i)
                    col[i] = get_column(line, pos);

                // code points range
                int cp0, cp1;
                const size_t ind = cpstr.find("..");
                if (ind == cpstr.npos) {
                    cp0 = hexstr_to_int(cpstr.data(), cpstr.data() + cpstr.length());
                    cp1 = cp0;
                } else {
                    cp0 = hexstr_to_int(cpstr.data(), cpstr.data() + ind);
                    cp1 = hexstr_to_int(cpstr.data() + ind + 2, cpstr.data() + cpstr.length());
                }

                outputFun(cp0, cp1, col);
            }
            catch (std::exception& ex) {
                std::cerr << "ERROR: " << ex.what() << std::endl;
                std::cerr << " LINE(" << line_num << "): " << line << std::endl;
            }
        }
    }
}

template <class T, class Compare = std::less<T>>
class array_view {
public:
    array_view(const array_view& src)
        : begin_(src.begin_)
        , end_(src.end_)
    {}
    array_view(const T* begin, size_t count)
        : begin_(begin)
        , end_(begin_ + count)
    {}

    const T* begin() const { return begin_; }
    const T* end() const { return end_; }
    size_t size() const { return end_ - begin_; }

    friend inline bool operator<(const array_view& lhs, const array_view& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), Compare());
    }
protected:
    const T* begin_;
    const T* end_;
};

struct block_info {
    template <class T, class Compare = std::less<T>>
    size_t calc_mem_size(const std::vector<T>& arrValues, size_t count, size_t value_size, int shift);

    template <class T, class Compare = std::less<T>>
    size_t calc_mem_size2(const std::vector<T>& arrValues, size_t count, size_t value_size, int shift);

    inline size_t total_mem() const {
        return blocks_mem + index_mem;
    }

    inline uint32_t code_point_mask() const {
        return 0xffffffff >> (32 - size_shift);
    }

    // input
    int size_shift;
    size_t block_size;
    // result
    size_t blocks_count;
    size_t blocks_mem;
    size_t index_count;
    size_t index_mem;
    size_t values_count;
};

template <class T, class Compare = std::less<T>>
inline block_info find_block_size(const std::vector<T>& arrValues, size_t count, size_t value_size = sizeof(T), int levels = 1) {
    size_t min_mem_size = static_cast<size_t>(-1);
    block_info min_bi;

    for (int size_shift = 1; size_shift < 16; size_shift++) {
        block_info bi;
        size_t mem_size =
            levels <= 1
            ? bi.calc_mem_size<T, Compare>(arrValues, count, value_size, size_shift)
            : bi.calc_mem_size2<T, Compare>(arrValues, count, value_size, size_shift);

        if (levels == 0) std::cout << "  ";
        std::cout << bi.block_size << "(" << bi.size_shift << ")" << ": "
            << mem_size << " = " << bi.blocks_mem << " + " << bi.index_mem
            << std::endl;

        if (min_mem_size > mem_size) {
            min_mem_size = mem_size;
            min_bi = std::move(bi);
        }
    }
    return min_bi;
}

template <class T, class Compare>
inline size_t block_info::calc_mem_size(const std::vector<T>& arrValues, size_t count, size_t value_size, int shift) {
    typedef array_view<T, Compare> array_view_T;

    size_shift = shift;
    block_size = static_cast<size_t>(1) << size_shift;

    // count blocks
    std::set<array_view_T> blocks;

    for (size_t ind = 0; ind < count; ind += block_size) {
        size_t chunk_size = std::min(block_size, arrValues.size() - ind);
        array_view_T block(arrValues.data() + ind, chunk_size);
        blocks.insert(block);
    }

    // blocks count & mem size
    blocks_count = blocks.size();
    blocks_mem = blocks_count * block_size * value_size;

    // index count & mem size
    index_count = (count / block_size) + (count % block_size != 0);
    if (blocks_count <= 0xFF) {
        index_mem = index_count;
    } else if (blocks_count <= 0xFFFF) {
        index_mem = index_count * 2;
    } else {
        index_mem = index_count * 4;
    }

    // stored values count
    values_count = std::min(index_count * block_size, arrValues.size());

    return total_mem();
}

template <class T, class Compare>
inline size_t block_info::calc_mem_size2(const std::vector<T>& arrValues, size_t count, size_t value_size, int shift) {
    typedef array_view<T, Compare> array_view_T;
    typedef std::map<array_view_T, int> BlokcsMap;

    size_shift = shift;
    block_size = static_cast<size_t>(1) << size_shift;

    // count blocks
    BlokcsMap blocks;
    std::vector<int> blockIndex;

    int index = 0;
    for (size_t ind = 0; ind < count; ind += block_size) {
        size_t chunk_size = std::min(block_size, arrValues.size() - ind);
        array_view_T block(arrValues.data() + ind, chunk_size);

        auto res = blocks.insert(BlokcsMap::value_type(block, index));
        if (res.second) {
            // for (const char_item& chitem : block) out(chitem.value);
            blockIndex.push_back(index);
            index++;
        } else {
            // index of previously inserted block
            blockIndex.push_back(res.first->second);
        }
    }

    // blocks count & mem size
    blocks_count = blocks.size();
    blocks_mem = blocks_count * block_size * value_size;

    // index count & mem size
    index_count = blockIndex.size();
    size_t index_item_size;
    if (blocks_count <= 0xFF) {
        index_item_size = 1;
    } else if (blocks_count <= 0xFFFF) {
        index_item_size = 2;
    } else {
        index_item_size = 4;
    }
    // second tier >>
    block_info ii = find_block_size(blockIndex, index_count, index_item_size, 0);
    index_mem = ii.total_mem();
    // << end tier

    // stored values count
    values_count = std::min(index_count * block_size, arrValues.size());

    return total_mem();
}

template <typename ValT>
struct special_ranges {
    using value_type = ValT;

    struct range_value {
        std::size_t from{}, to{};
        value_type value{};

        template <class T>
        range_value(const std::vector<T>& values_arr, std::size_t ind)
            : from{ ind }
            , to{ ind }
            , value{ values_arr[ind] }
        {}
    };
    std::vector<range_value> m_range;

    template <class T>
    special_ranges(const std::vector<T>& values_arr, std::size_t max_range_count = 1) {
        if (values_arr.size() > 0) {
            // add main range
            m_range.emplace_back(values_arr, values_arr.size() - 1);
            range_value* r = &m_range[0];

            for (std::ptrdiff_t ind = static_cast<std::ptrdiff_t>(values_arr.size()) - 2; ind >= 0; --ind) {
                value_type val = values_arr[ind];
                if (val == r->value) {
                    r->from = ind;
                } else if (val == m_range[0].value) {
                    // fall back to main range
                    r = &m_range[0];
                    r->from = ind;
                } else if (m_range.size() < max_range_count) {
                    // add new range
                    m_range.emplace_back(values_arr, static_cast<std::size_t>(ind));
                    r = std::addressof(m_range.back());
                } else
                    break;
            }
            // main range includes other ranges
            if (m_range.size() >= 2 && m_range[0].from > m_range.back().from)
                m_range[0].from = m_range.back().from;
        }
    }
};

// Output result

// OutputFmt class

class OutputFmt {
public:
    enum class Style { NONE = 0, CPP, JS };

    OutputFmt(std::ostream& fout, size_t max_line_len, Style style = Style::NONE);
    ~OutputFmt();

    void output(const std::string& item);
    template <typename UIntT>
    void output(UIntT num, int base);
protected:
    std::ostream& m_fout;
    bool m_first;
    size_t m_line_len;
    // options
    Style m_style;
    const size_t m_max_line_len;
    static const size_t m_indent = 2;
};

inline OutputFmt::OutputFmt(std::ostream& fout, size_t max_line_len, Style style)
    : m_fout(fout)
    , m_first(true)
    , m_line_len(0)
    , m_style(style)
    , m_max_line_len(max_line_len)
{
    static const char* strOpen[3] = { "", "{", "[" };
    m_fout << strOpen[static_cast<int>(m_style)] << std::endl;
}

inline OutputFmt::~OutputFmt() {
    static const char* strClose[3] = { "", "}", "]" };
    m_fout << strClose[static_cast<int>(m_style)] << std::endl;
}

inline void OutputFmt::output(const std::string& item) {
    if (m_first) {
        m_first = false;
        m_fout << std::setw(m_indent) << "";
        m_line_len = m_indent;
    }
    else if (m_line_len + item.length() > (m_max_line_len - 3)) {
        // 3 == ", ,".length
        m_fout << ",\n";
        m_fout << std::setw(m_indent) << "";
        m_line_len = m_indent;
    }
    else {
        m_fout << ", ";
        m_line_len += 2;
    }
    m_fout << item;
    m_line_len += item.length();
}

template <typename UIntT>
inline void OutputFmt::output(UIntT num, int base) {
    std::string strNum;
    unsigned_to_numstr<UIntT>(num, strNum, static_cast<UIntT>(base));
    output(strNum);
}


} // namespace tools
} // namespace upa
