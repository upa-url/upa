// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_STR_VIEW_H
#define UPA_STR_VIEW_H

#include "config.h"
#include <algorithm>
#include <string>
#include <type_traits>

namespace upa {

/// @brief A very simplified implementation of the basic_string_view
///
/// It is used when compiling with C++11 or C++14 compilers.
template<typename CharT, typename Traits = std::char_traits<CharT>>
class str_view {
public:
    // types
    using traits_type = Traits;
    using value_type = CharT;
    using pointer = CharT*;
    using const_pointer = const CharT*;
    using reference = CharT&;
    using const_reference = const CharT&;
    using const_iterator = const CharT*; // implementation-defined; see 21.4.2.2 in C++20 standard
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    // static constexpr size_type npos = size_type(-1);

    // constructors
    constexpr str_view() noexcept = default;
    constexpr str_view(const str_view&) noexcept = default;
    constexpr str_view(const CharT* ptr, size_type len) noexcept : ptr_(ptr), len_(len) {}
    str_view(const CharT* ptr) : ptr_(ptr), len_(Traits::length(ptr)) {}

    // assignment
    UPA_CONSTEXPR_14 str_view& operator=(const str_view&) noexcept = default;

    // destructor
    ~str_view() noexcept = default;

    // iterator support
    constexpr const_iterator begin() const noexcept { return ptr_; }
    constexpr const_iterator end() const noexcept { return ptr_ + len_; }

    // capacity
    constexpr size_type size() const noexcept { return len_; }
    constexpr size_type length() const noexcept { return len_; }
    constexpr bool empty() const noexcept { return len_ == 0; }

    // element access
    constexpr const_reference operator[](size_type ind) const {
        return ptr_[ind];
    }
    constexpr const_pointer data() const noexcept { return ptr_; }

    // modifiers
    UPA_CONSTEXPR_14 void remove_prefix(size_type n) {
        ptr_ += n;
        len_ -= n;
    }
    UPA_CONSTEXPR_14 void remove_suffix(size_type n) {
        len_ -= n;
    }
    UPA_CONSTEXPR_14 void swap(str_view& x) noexcept {
        const str_view tmp{x};
        x = *this;
        *this = tmp;
    }

    // string operations
    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }


    // non-standard
    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    // basic_string, basic_string_view support
    template <class StrT, typename std::enable_if<std::is_same<typename StrT::value_type, value_type>::value, int>::type = 0>
    str_view(const StrT& str) : ptr_(str.data()), len_(str.length()) {}

    //template <class StrT, typename std::enable_if<std::is_same<typename StrT::value_type, value_type>::value, int>::type = 0>
    //operator StrT() const { return StrT(ptr_, len_); }
    template <class Allocator>
    operator std::basic_string<CharT, Traits, Allocator>() const {
        return std::basic_string<CharT, Traits, Allocator>(ptr_, len_);
    }

private:
    const_pointer ptr_ = nullptr;
    size_type len_ = 0;
};


// compare two str_view
template<class CharT, class Traits>
/*constexpr*/ bool operator==(str_view<CharT, Traits> lhs, str_view<CharT, Traits> rhs) /*noexcept*/ {
    return lhs.equal(rhs);
}

template<class CharT, class Traits>
/*constexpr*/ bool operator!=(str_view<CharT, Traits> lhs, str_view<CharT, Traits> rhs) /*noexcept*/ {
    return !lhs.equal(rhs);
}

// compare objects convertible to str_view for equality
template<class CharT, class Traits, class StrT,
    typename std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value, int>::type = 0>
/*constexpr*/ bool operator==(StrT&& lhs, const str_view<CharT, Traits> rhs) {
    return rhs.equal(std::forward<StrT>(lhs));
}

template<class CharT, class Traits, class StrT,
    typename std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value, int>::type = 0>
/*constexpr*/ bool operator==(const str_view<CharT, Traits> lhs, StrT&& rhs) {
    return lhs.equal(std::forward<StrT>(rhs));
}

// stream output
template <class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
    str_view<CharT, Traits> v) {
    // TODO FormattedOutputFunction
    return os.write(v.data(), v.length());
}


} // namespace upa

#endif // UPA_STR_VIEW_H
