#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <algorithm>
#include <string>
#include <type_traits>


template<typename CharT, typename Traits = std::char_traits<CharT>>
class str_view {
public:
    str_view(const str_view& sv) : ptr_(sv.ptr_), len_(sv.len_) {}
    str_view(const CharT* ptr, std::size_t len) : ptr_(ptr), len_(len) {}
    str_view(const CharT* ptr) : ptr_(ptr), len_(Traits::length(ptr)) {}
    str_view(const std::basic_string<CharT>& str) : ptr_(str.data()), len_(str.length()) {}

    const CharT* data() const { return ptr_; }
    std::size_t length() const { return len_; }
    std::size_t size() const { return len_; }
    const CharT* begin() const { return ptr_; }
    const CharT* end() const { return ptr_ + len_; }

    CharT operator[](std::size_t ind) const {
        return ptr_[ind];
    }

    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }

    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    void append_to(std::basic_string<CharT>& str) const {
        str.append(ptr_, len_);
    }

    void remove_prefix(size_t n) {
        ptr_ += n;
        len_ -= n;
    }
    void remove_suffix(size_t n) {
        len_ -= n;
    }

private:
    const CharT* ptr_;
    std::size_t len_;
};


#endif // STR_VIEW_H
