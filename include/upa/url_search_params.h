// Copyright 2016-2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_SEARCH_PARAMS_H
#define UPA_URL_SEARCH_PARAMS_H

#include "config.h"
#include "str_arg.h"
#include "url_percent_encode.h"
#include "url_utf.h"
#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace upa {


namespace detail {

// is key value pair
template <typename>
struct is_pair : std::false_type {};

template<class T1, class T2>
struct is_pair<std::pair<T1, T2>> : std::true_type {};

// Get iterable's value type
// https://stackoverflow.com/a/29634934
template <typename T>
auto iterable_value(int) -> decltype(
    std::begin(std::declval<T&>()) != std::end(std::declval<T&>()), // begin/end and operator !=
    ++std::declval<decltype(std::begin(std::declval<T&>()))&>(), // operator ++
    *std::begin(std::declval<T&>()) // operator *
);
template <typename T>
auto iterable_value(long) -> void;

template<class T>
using iterable_value_t = std::remove_cv_t<std::remove_reference_t<
    decltype(iterable_value<T>(0))
>>;

// is iterable over the std::pair values
template<class T>
constexpr bool is_iterable_pairs_v = is_pair<iterable_value_t<T>>::value;

// enable if `Base` is not the base class of `T`
template<class Base, class T>
using enable_if_not_base_of_t = std::enable_if_t<
    !std::is_base_of_v<Base, std::decay_t<T>>, int
>;

} // namespace detail


// forward declarations

class url;
namespace detail {
    class url_search_params_ptr;
} // namespace detail

/// @brief URLSearchParams class
///
/// Follows specification in
/// https://url.spec.whatwg.org/#interface-urlsearchparams
///
class url_search_params
{
public:
    // types
    using name_value_pair = std::pair<std::string, std::string>;
    using name_value_list = std::list<name_value_pair>;
    using const_iterator = name_value_list::const_iterator;
    using const_reverse_iterator = name_value_list::const_reverse_iterator;
    using iterator = const_iterator;
    using reverse_iterator = const_reverse_iterator;
    using size_type = name_value_list::size_type;
    using value_type = name_value_pair;

    // Constructors

    /// @brief Default constructor.
    ///
    /// Constructs empty @c url_search_params object.
    url_search_params() = default;

    /// @brief Copy constructor.
    ///
    /// @param[in] other @c url_search_params object to copy from
    url_search_params(const url_search_params& other);

    /// @brief Move constructor.
    ///
    /// Constructs the @c url_search_params object with the contents of @a other
    /// using move semantics.
    ///
    /// @param[in,out] other @c url_search_params object to move from
    url_search_params(url_search_params&& other)
        noexcept(std::is_nothrow_move_constructible_v<name_value_list>);

    /// @brief Parsing constructor.
    ///
    /// Initializes name-value pairs list by parsing query string.
    ///
    /// @param[in] query string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    explicit url_search_params(StrT&& query)
        : params_(do_parse(true, std::forward<StrT>(query)))
    {}

    /// Initializes name-value pairs list by copying pairs fron container.
    ///
    /// @param[in] cont name-value pairs container
    template<class ConT,
        // do not hide the copy and move constructors:
        detail::enable_if_not_base_of_t<url_search_params, ConT> = 0,
        std::enable_if_t<detail::is_iterable_pairs_v<ConT>, int> = 0
    >
    explicit url_search_params(ConT&& cont) {
        for (const auto& p : cont) {
            params_.emplace_back(make_string(p.first), make_string(p.second));
        }
    }

    /// destructor
    ~url_search_params() = default;

    // Assignment

    /// @brief Copy assignment.
    ///
    /// Replaces the contents with those of @a other using copy semantics.
    ///
    /// Updates linked URL object if any. So this operator can be used to assign
    /// a value to the reference returned by url::search_params().
    ///
    /// @param[in] other  url_search_params to copy from
    /// @return *this
    url_search_params& operator=(const url_search_params& other);

    /// @brief Move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    ///
    /// NOTE: It is undefined behavior to use this operator to assign a value to
    /// the reference returned by url::search_params(). Use safe_assign instead.
    ///
    /// @param[in,out] other url_search_params to move to this object
    /// @return *this
    url_search_params& operator=(url_search_params&& other) noexcept;

    /// @brief Safe move assignment.
    ///
    /// Replaces the contents with those of @a other using move semantics.
    ///
    /// Updates linked URL object if any. So this function can be used to assign
    /// a value to the reference returned by url::search_params().
    ///
    /// @param[in,out] other  URL to move to this object
    /// @return *this
    url_search_params& safe_assign(url_search_params&& other);

    // Operations

    /// @brief Clears parameters
    void clear();

    /// @brief Swaps the contents of two url_search_params
    ///
    /// NOTE: It is undefined behavior to use this function to swap the
    /// contents of references returned by url::search_params().
    ///
    /// @param[in,out] other url_search_params to exchange the contents with
    void swap(url_search_params& other) noexcept;

    /// Initializes name-value pairs list by parsing query string.
    ///
    /// @param[in] query string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    void parse(StrT&& query);

    /// Appends given name-value pair to list
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-append
    ///
    /// @param[in] name
    /// @param[in] value
    template <class TN, class TV>
    void append(TN&& name, TV&& value);

    /// Remove all name-value pairs whose name is @a name from list.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-delete
    ///
    /// @param[in] name
    template <class TN>
    void del(const TN& name);

    /// Remove all name-value pairs whose name is @a name and value is @a value from list.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-delete
    ///
    /// @param[in] name
    /// @param[in] value
    template <class TN, class TV>
    void del(const TN& name, const TV& value);

    /// Remove all name-value pairs whose name is @a name from list.
    ///
    /// It updates connected URL only if something is removed.
    ///
    /// @param[in] name
    /// @return the number of pairs removed
    template <class TN>
    size_type remove(const TN& name);

    /// Remove all name-value pairs whose name is @a name and value is @a value from list.
    ///
    /// It updates connected URL only if something is removed.
    ///
    /// @param[in] name
    /// @param[in] value
    /// @return the number of pairs removed
    template <class TN, class TV>
    size_type remove(const TN& name, const TV& value);

    /// Remove all name-value pairs for which predicate @a p returns `true`.
    ///
    /// It updates connected URL only if something is removed.
    ///
    /// @param[in] p unary predicate which returns value of `bool` type and has
    ///   `const value_type&` parameter
    /// @return the number of pairs removed
    template <class UnaryPredicate>
    size_type remove_if(UnaryPredicate p);

    /// Returns value of the first name-value pair whose name is @a name, or `nullptr`,
    /// if there isn't such pair.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-get
    ///
    /// @param[in] name
    /// @return pair value, or `nullptr`
    template <class TN>
    [[nodiscard]] const std::string* get(const TN& name) const;

    /// Return the values of all name-value pairs whose name is @a name.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-getall
    ///
    /// @param[in] name
    /// @return list of values as `std::string`
    template <class TN>
    [[nodiscard]] std::list<std::string> get_all(const TN& name) const;

    /// Tests if list contains a name-value pair whose name is @a name.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-has
    ///
    /// @param[in] name
    /// @return `true`, if list contains such pair, `false` otherwise
    template <class TN>
    [[nodiscard]] bool has(const TN& name) const;

    /// Tests if list contains a name-value pair whose name is @a name and value is @a value.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-has
    ///
    /// @param[in] name
    /// @param[in] value
    /// @return `true`, if list contains such pair, `false` otherwise
    template <class TN, class TV>
    [[nodiscard]] bool has(const TN& name, const TV& value) const;

    /// Sets search parameter value
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-set
    ///
    /// @param[in] name
    /// @param[in] value
    template <class TN, class TV>
    void set(TN&& name, TV&& value);

    /// Sort all name-value pairs, by their names.
    ///
    /// Sorting is done by comparison of code units. The relative order between
    /// name-value pairs with equal names is preserved.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-sort
    void sort();

    /// Serializes name-value pairs to string and appends it to @a query.
    ///
    /// @param[in,out] query
    void serialize(std::string& query) const;

    /// Serializes name-value pairs to string.
    ///
    /// More info: https://url.spec.whatwg.org/#urlsearchparams-stringification-behavior
    ///
    /// @return serialized name-value pairs
    [[nodiscard]] std::string to_string() const;

    // Iterators

    /// @return an iterator to the beginning of name-value list
    [[nodiscard]] const_iterator begin() const noexcept { return params_.begin(); }

    /// @return an iterator to the beginning of name-value list
    [[nodiscard]] const_iterator cbegin() const noexcept { return params_.cbegin(); }

    /// @return an iterator to the end of name-value list
    [[nodiscard]] const_iterator end() const noexcept { return params_.end(); }

    /// @return an iterator to the end of name-value list
    [[nodiscard]] const_iterator cend() const noexcept { return params_.cend(); }

    /// @return a reverse iterator to the beginning of name-value list
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return params_.rbegin(); }

    /// @return a reverse iterator to the beginning of name-value list
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return params_.crbegin(); }

    /// @return a reverse iterator to the end of name-value list
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return params_.rend(); }

    /// @return a reverse iterator to the end of name-value list
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return params_.crend(); }

    // Capacity

    /// Checks whether the name-value list is empty
    ///
    /// @return `true` if the container is empty, `false` otherwise
    [[nodiscard]] bool empty() const noexcept { return params_.empty(); }

    /// @return the number of elements in the name-value list
    [[nodiscard]] size_type size() const noexcept { return params_.size(); }

    // Utils

    /// Initializes and returns name-value pairs list by parsing query string.
    ///
    /// @param[in] rem_qmark if it is `true` and the @a query starts with U+003F (?),
    ///   then skips first code point in @a query
    /// @param[in] query string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    [[nodiscard]] static name_value_list do_parse(bool rem_qmark, StrT&& query);

    /// Percent encodes the @a value using application/x-www-form-urlencoded percent-encode set,
    /// and replacing 0x20 (SP) with U+002B (+). Appends result to the @a encoded string.
    ///
    /// @param[in,out] encoded string to append percent encoded value
    /// @param[in] value string value to percent encode
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    static void urlencode(std::string& encoded, StrT&& value);

private:
    explicit url_search_params(url* url_ptr);

    void clear_params() noexcept;
    void copy_params(const url_search_params& other);
    void move_params(url_search_params&& other) noexcept;
    void parse_params(string_view query);

    void update();

    friend class url;
    friend class detail::url_search_params_ptr;

private:
    name_value_list params_;
    bool is_sorted_ = false;
    url* url_ptr_ = nullptr;

    static const char kEncByte[0x100];
};


namespace detail {

class url_search_params_ptr
{
public:
    url_search_params_ptr() noexcept = default;

    // copy constructor initializes to nullptr
    url_search_params_ptr(const url_search_params_ptr&) noexcept {}
    url_search_params_ptr& operator=(const url_search_params_ptr& other);

    // move constructor/assignment
    url_search_params_ptr(url_search_params_ptr&& other) noexcept = default;
    url_search_params_ptr& operator=(url_search_params_ptr&& other) noexcept = default;

    // destructor
    ~url_search_params_ptr() = default;

    void init(url* url_ptr) {
        ptr_.reset(new url_search_params(url_ptr)); // NOLINT(cppcoreguidelines-owning-memory)
    }

    void set_url_ptr(url* url_ptr) noexcept {
        if (ptr_)
            ptr_->url_ptr_ = url_ptr;
    }

    void clear_params() noexcept {
        assert(ptr_);
        ptr_->clear_params();
    }
    void parse_params(string_view query) {
        assert(ptr_);
        ptr_->parse_params(query);
    }

    explicit operator bool() const noexcept {
        return static_cast<bool>(ptr_);
    }
    url_search_params& operator*() const {
        return *ptr_;
    }
    url_search_params* operator->() const noexcept {
        return ptr_.get();
    }
private:
    std::unique_ptr<url_search_params> ptr_;
};

} // namespace detail


// url_search_params inline

// Copy constructor

inline url_search_params::url_search_params(const url_search_params& other)
    : params_(other.params_)
    , is_sorted_(other.is_sorted_)
{}

// Move constructor

inline url_search_params::url_search_params(url_search_params&& other)
    noexcept(std::is_nothrow_move_constructible_v<name_value_list>)
    : params_(std::move(other.params_))
    , is_sorted_(other.is_sorted_)
{}

// Assignment

inline url_search_params& url_search_params::operator=(const url_search_params& other) {
    if (this != std::addressof(other)) {
        copy_params(other);
        update();
    }
    return *this;
}

inline url_search_params& url_search_params::operator=(url_search_params&& other) noexcept {
    assert(url_ptr_ == nullptr);
    move_params(std::move(other));
    return *this;
}

inline url_search_params& url_search_params::safe_assign(url_search_params&& other) {
    move_params(std::move(other));
    update();
    return *this;
}

// Operations

inline void url_search_params::clear() {
    params_.clear();
    is_sorted_ = true;
    update();
}

inline void url_search_params::swap(url_search_params& other) noexcept {
    assert(url_ptr_ == nullptr && other.url_ptr_ == nullptr);

    using std::swap;

    swap(params_, other.params_);
    swap(is_sorted_, other.is_sorted_);
}

inline void url_search_params::clear_params() noexcept {
    params_.clear();
    is_sorted_ = true;
}

inline void url_search_params::copy_params(const url_search_params& other) {
    params_ = other.params_;
    is_sorted_ = other.is_sorted_;
}

inline void url_search_params::move_params(url_search_params&& other) noexcept {
    params_ = std::move(other.params_);
    is_sorted_ = other.is_sorted_;
}

inline void url_search_params::parse_params(string_view query) {
    params_ = do_parse(false, query);
    is_sorted_ = false;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline void url_search_params::parse(StrT&& query) {
    params_ = do_parse(true, std::forward<StrT>(query));
    is_sorted_ = false;
    update();
}

template <class TN, class TV>
inline void url_search_params::append(TN&& name, TV&& value) {
    params_.emplace_back(
        make_string(std::forward<TN>(name)),
        make_string(std::forward<TV>(value))
    );
    is_sorted_ = false;
    update();
}

template <class TN>
inline void url_search_params::del(const TN& name) {
    const auto str_name = make_string(name);

    params_.remove_if([&](const value_type& item) {
        return item.first == str_name;
    });
    update();
}

template <class TN, class TV>
inline void url_search_params::del(const TN& name, const TV& value) {
    const auto str_name = make_string(name);
    const auto str_value = make_string(value);

    params_.remove_if([&](const value_type& item) {
        return item.first == str_name && item.second == str_value;
    });
    update();
}

template <class TN>
inline url_search_params::size_type url_search_params::remove(const TN& name) {
    const auto str_name = make_string(name);

    return remove_if([&](const value_type& item) {
        return item.first == str_name;
    });
}

template <class TN, class TV>
inline url_search_params::size_type url_search_params::remove(const TN& name, const TV& value) {
    const auto str_name = make_string(name);
    const auto str_value = make_string(value);

    return remove_if([&](const value_type& item) {
        return item.first == str_name && item.second == str_value;
    });
}

template <class UnaryPredicate>
inline url_search_params::size_type url_search_params::remove_if(UnaryPredicate p) {
#ifdef __cpp_lib_list_remove_return_type
    const size_type count = params_.remove_if(p);
#else
    const size_type old_size = params_.size();
    params_.remove_if(p);
    const size_type count = old_size - params_.size();
#endif
    if (count) update();
    return count;
}

template <class TN>
inline const std::string* url_search_params::get(const TN& name) const {
    const auto str_name = make_string(name);
    for (const auto& p : params_) {
        if (p.first == str_name)
            return &p.second;
    }
    return nullptr;
}

template <class TN>
inline std::list<std::string> url_search_params::get_all(const TN& name) const {
    std::list<std::string> lst;
    const auto str_name = make_string(name);
    for (const auto& p : params_) {
        if (p.first == str_name)
            lst.push_back(p.second);
    }
    return lst;
}

template <class TN>
inline bool url_search_params::has(const TN& name) const {
    const auto str_name = make_string(name);
    for (const auto& p : params_) {
        if (p.first == str_name)
            return true;
    }
    return false;
}

template <class TN, class TV>
inline bool url_search_params::has(const TN& name, const TV& value) const {
    const auto str_name = make_string(name);
    const auto str_value = make_string(value);

    for (const auto& p : params_) {
        if (p.first == str_name && p.second == str_value)
            return true;
    }
    return false;
}

template <class TN, class TV>
inline void url_search_params::set(TN&& name, TV&& value) {
    auto str_name = make_string(std::forward<TN>(name));
    auto str_value = make_string(std::forward<TV>(value));

    bool is_match = false;
    for (auto it = params_.begin(); it != params_.end(); ) {
        if (it->first == str_name) {
            if (is_match) {
                it = params_.erase(it);
                continue;
            }
            it->second = std::move(str_value);
            is_match = true;
        }
        ++it;
    }
    if (!is_match)
        append(std::move(str_name), std::move(str_value));
    else
        update();
}

inline void url_search_params::sort() {
    // https://url.spec.whatwg.org/#dom-urlsearchparams-sort
    // Sorting must be done by comparison of code units. The relative order
    // between name-value pairs with equal names must be preserved.
    if (!is_sorted_) {
        // https://en.cppreference.com/w/cpp/container/list/sort
        // std::list::sort preserves the order of equal elements.
        params_.sort([](const name_value_pair& a, const name_value_pair& b) {
            //return a.first < b.first;
            return url_utf::compare_by_code_units(
                a.first.data(), a.first.data() + a.first.size(),
                b.first.data(), b.first.data() + b.first.size()) < 0;
        });
        is_sorted_ = true;
    }
    update();
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline url_search_params::name_value_list url_search_params::do_parse(bool rem_qmark, StrT&& query) {
    name_value_list lst;

    const auto str_query = make_string(std::forward<StrT>(query));
    auto b = str_query.begin();
    const auto e = str_query.end();

    // remove leading question-mark?
    if (rem_qmark && b != e && *b == '?')
        ++b;

    std::string name;
    std::string value;
    std::string* pval = &name;
    auto start = b;
    for (auto it = b; it != e; ++it) {
        switch (*it) {
        case '=':
            if (pval != &value)
                pval = &value;
            else
                pval->push_back(*it);
            break;
        case '&':
            if (start != it) {
                url_utf::check_fix_utf8(name);
                url_utf::check_fix_utf8(value);
                lst.emplace_back(std::move(name), std::move(value));
                // clear after move
                name.clear();
                value.clear();
            }
            pval = &name;
            start = it + 1; // skip '&'
            break;
        case '+':
            pval->push_back(' ');
            break;
        case '%':
            if (std::distance(it, e) > 2) {
                auto itc = it;
                const auto uc1 = static_cast<unsigned char>(*(++itc));
                const auto uc2 = static_cast<unsigned char>(*(++itc));
                if (detail::is_hex_char(uc1) && detail::is_hex_char(uc2)) {
                    const char c = static_cast<char>((detail::hex_char_to_num(uc1) << 4) + detail::hex_char_to_num(uc2));
                    pval->push_back(c);
                    it = itc;
                    break;
                }
            }
            [[fallthrough]];
        default:
            pval->push_back(*it);
            break;
        }
    }
    if (start != e) {
        url_utf::check_fix_utf8(name);
        url_utf::check_fix_utf8(value);
        lst.emplace_back(std::move(name), std::move(value));
    }
    return lst;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline void url_search_params::urlencode(std::string& encoded, StrT&& value) {
    const auto str_value = make_string(std::forward<StrT>(value));

    for (const char c : str_value) {
        const auto uc = static_cast<unsigned char>(c);
        const char cenc = kEncByte[uc];
        encoded.push_back(cenc);
        if (cenc == '%') {
            encoded.push_back(detail::kHexCharLookup[uc >> 4]);
            encoded.push_back(detail::kHexCharLookup[uc & 0xF]);
        }
    }
}

inline void url_search_params::serialize(std::string& query) const {
    auto it = params_.begin();
    if (it != params_.end()) {
        while (true) {
            urlencode(query, it->first); // name
            query.push_back('=');
            urlencode(query, it->second); // value
            if (++it == params_.end())
                break;
            query.push_back('&');
        }
    }
}

inline std::string url_search_params::to_string() const {
    std::string query;
    serialize(query);
    return query;
}

// Non-member functions

/// @brief Swaps the contents of two url_search_params
///
/// Swaps the contents of the @a lhs and @a rhs url_search_params
///
/// NOTE: It is undefined behavior to use this function to swap the
/// contents of references returned by url::search_params().
///
/// @param[in,out] lhs
/// @param[in,out] rhs
inline void swap(url_search_params& lhs, url_search_params& rhs) noexcept {
    lhs.swap(rhs);
}


} // namespace upa

#endif // UPA_URL_SEARCH_PARAMS_H
