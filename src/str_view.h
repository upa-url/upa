#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <algorithm>
#include <string>
#include <type_traits>


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
    using const_iterator = const CharT*; // implementation-defined; see 21.4.2.2
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    // static constexpr size_type npos = size_type(-1);

    // construction and assignment
    str_view() : ptr_(nullptr), len_(0) {}
    str_view(const str_view&) = default;
    str_view& operator=(const str_view&) = default;
    str_view(const CharT* ptr, size_type len) : ptr_(ptr), len_(len) {}
    str_view(const CharT* ptr) : ptr_(ptr), len_(Traits::length(ptr)) {}

    // iterator support
    const_iterator begin() const { return ptr_; }
    const_iterator end() const { return ptr_ + len_; }

    // capacity
    size_type size() const { return len_; }
    size_type length() const { return len_; }
    bool empty() const { return len_ == 0; }

    // element access
    const_reference operator[](size_type ind) const {
        return ptr_[ind];
    }
    const_pointer data() const { return ptr_; }

    // modifiers
    void remove_prefix(size_type n) {
        ptr_ += n;
        len_ -= n;
    }
    void remove_suffix(size_type n) {
        len_ -= n;
    }
    void swap(str_view& x) {
        const str_view tmp{x};
        x = *this;
        *this = tmp;
    }

    // string operations
    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }


    // non-standart
    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    // basic_string, basic_string_view support
    template <class StrT, typename = typename std::enable_if<std::is_same<typename StrT::value_type, value_type>::value>::type>
    str_view(const StrT& str) : ptr_(str.data()), len_(str.length()) {}

    //template <class StrT, typename = typename std::enable_if<std::is_same<typename StrT::value_type, value_type>::value>::type>
    //operator StrT() const { return StrT(ptr_, len_); }
    template <class Allocator>
    operator std::basic_string<CharT, Traits, Allocator>() const {
        return std::basic_string<CharT, Traits, Allocator>(ptr_, len_);
    }

private:
    const_pointer ptr_;
    size_type len_;
};


// compare two str_view
template<class CharT, class Traits>
/*constexpr*/ bool operator==(str_view<CharT, Traits> lhs, str_view<CharT, Traits> rhs) /*noexcept*/ {
    return lhs.equal(rhs);
}

// compare objects convertible to str_view for equality
template<class CharT, class Traits, class StrT,
class = typename std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value>::type>
/*constexpr*/ bool operator==(StrT&& lhs, const str_view<CharT, Traits> rhs) {
    return rhs.equal(std::forward<StrT>(lhs));
}

template<class CharT, class Traits, class StrT,
class = typename std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value>::type>
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

#endif // STR_VIEW_H
