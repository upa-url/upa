// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_SEARCH_PARAMS_H
#define WHATWG_URL_SEARCH_PARAMS_H

#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "str_arg.h"
#include "url_percent_encode.h"

namespace whatwg {


namespace {

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
using iterable_value_t = typename std::remove_cv<typename std::remove_reference<
    decltype(iterable_value<T>(0))
>::type>::type;

// is iterable over the std::pair values
template<class T>
struct is_iterable_pairs : is_pair<iterable_value_t<T>> {};

}


// forward declarations

class url;
namespace detail {
    class url_search_params_ptr;
}

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
    url_search_params();

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
    url_search_params(url_search_params&& other) noexcept = default;

    /// @brief Parsing constructor.
    ///
    /// Initializes name-value pairs list by parsing query string.
    ///
    /// @param[in] query string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    url_search_params(StrT&& query);

    /// Initializes name-value pairs list by copying pairs fron container.
    ///
    /// @param[in] cont name-value pairs container
    template<class ConT, typename std::enable_if<is_iterable_pairs<ConT>::value, int>::type = 0>
    url_search_params(ConT&& cont);

    // Operations

    /// @brief Clears parameters
    void clear();

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

    /// Returns value of the first name-value pair whose name is @a name, or `nullptr`,
    /// if there isn't such pair.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-get
    ///
    /// @param[in] name
    /// @return pair value, or `nullptr`
    template <class TN>
    const std::string* get(const TN& name) const;

    /// Return the values of all name-value pairs whose name is @a name.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-getall
    ///
    /// @param[in] name
    /// @return list of values as `std::string`
    template <class TN>
    std::list<std::string> getAll(const TN& name) const;

    /// Tests if list contains a name-value pair whose name is @a name.
    ///
    /// More info: https://url.spec.whatwg.org/#dom-urlsearchparams-has
    ///
    /// @param[in] name
    /// @return `true`, if list contains such pair, `false` otherwise
    template <class TN>
    bool has(const TN& name) const;

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
    std::string to_string() const;

    // Iterable

    /// @return an iterator to the beginning of name-value list
    const_iterator begin() const { return params_.begin(); }

    /// @return an iterator to the beginning of name-value list
    const_iterator cbegin() const { return params_.cbegin(); }

    /// @return an iterator to the end of name-value list
    const_iterator end() const { return params_.end(); }

    /// @return an iterator to the end of name-value list
    const_iterator cend() const { return params_.cend(); }

    /// @return a reverse iterator to the beginning of name-value list
    const_reverse_iterator rbegin() const { return params_.rbegin(); }

    /// @return a reverse iterator to the beginning of name-value list
    const_reverse_iterator crbegin() const { return params_.crbegin(); }

    /// @return a reverse iterator to the end of name-value list
    const_reverse_iterator rend() const { return params_.rend(); }

    /// @return a reverse iterator to the end of name-value list
    const_reverse_iterator crend() const { return params_.crend(); }

    // Capacity

    /// Checks whether the name-value list is empty
    ///
    /// @return `true` if the container is empty, `false` otherwise
    bool empty() const noexcept { return params_.empty(); }

    /// @return the number of elements in the name-value list
    size_type size() const noexcept { return params_.size(); }

    // Utils

    /// Initializes and returns name-value pairs list by parsing query string.
    ///
    /// @param[in] rem_qmark if it is `true` and the @a query starts with U+003F (?),
    ///   then skips first code point in @a query
    /// @param[in] query string to parse
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    static name_value_list do_parse(bool rem_qmark, StrT&& query);

    /// Percent encodes the @a value using application/x-www-form-urlencoded percent-encode set,
    /// and replacing 0x20 (SP) with U+002B (+). Appends result to the @a encoded string.
    ///
    /// @param[in,out] encoded string to append percent encoded value
    /// @param[in] value string value to percent encode
    template <class StrT, enable_if_str_arg_t<StrT> = 0>
    static void urlencode(std::string& encoded, StrT&& value);

private:
    explicit url_search_params(url* url_ptr);

    void clear_params();
    void parse_params(url_str_view_t query);

    void update();

    friend detail::url_search_params_ptr;

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
    url_search_params_ptr(url_search_params_ptr&&) noexcept = default;

    // move assignment
    url_search_params_ptr& operator=(url_search_params_ptr&& other) noexcept {
        ptr_ = std::move(other.ptr_);
        return *this;
    }

    // copy constructor/assignment initializes to nullptr
    url_search_params_ptr(const url_search_params_ptr&) noexcept {}
    url_search_params_ptr& operator=(const url_search_params_ptr&) noexcept {
        ptr_ = nullptr;
        return *this;
    }

    void init(url* url_ptr) {
        ptr_.reset(new url_search_params(url_ptr));
    }

    void clear_params() {
        assert(ptr_);
        ptr_->clear_params();
    }
    void parse_params(url_str_view_t query) {
        assert(ptr_);
        ptr_->parse_params(query);
    }

    explicit operator bool() const noexcept {
        return bool(ptr_);
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

inline url_search_params::url_search_params()
{}

inline url_search_params::url_search_params(const url_search_params& other)
    : params_(other.params_)
    , is_sorted_(other.is_sorted_)
    , url_ptr_(nullptr)
{}

// https://url.spec.whatwg.org/#dom-urlsearchparams-urlsearchparams
template <class StrT, enable_if_str_arg_t<StrT>>
inline url_search_params::url_search_params(StrT&& query)
    : params_(do_parse(true, std::forward<StrT>(query)))
{}

template<class ConT, typename std::enable_if<is_iterable_pairs<ConT>::value, int>::type>
inline url_search_params::url_search_params(ConT&& cont) {
    for (const auto& p : cont) {
        params_.emplace_back(make_string(p.first), make_string(p.second));
    }
}

// Operations

inline void url_search_params::clear() {
    params_.clear();
    is_sorted_ = true;
    update();
}

inline void url_search_params::clear_params() {
    params_.clear();
    is_sorted_ = true;
}

inline void url_search_params::parse_params(url_str_view_t query) {
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
    for (auto it = params_.begin(); it != params_.end();) {
        if (it->first == str_name)
            it = params_.erase(it);
        else
            ++it;
    }
    update();
}

template <class TN>
inline const std::string* url_search_params::get(const TN& name) const {
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
            return &it->second;
    }
    return nullptr;
}

template <class TN>
std::list<std::string> url_search_params::getAll(const TN& name) const {
    std::list<std::string> lst;
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
            lst.push_back(it->second);
    }
    return lst;
}

template <class TN>
inline bool url_search_params::has(const TN& name) const {
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
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

    auto str_query = make_string(std::forward<StrT>(query));
    auto b = str_query.begin();
    auto e = str_query.end();

    // remove leading question-mark?
    if (rem_qmark && b != e && *b == '?')
        ++b;

    name_value_pair p;
    std::string* pval = &p.first;
    auto start = b;
    for (auto it = b; it != e; ++it) {
        switch (*it) {
        case '=':
            if (pval != &p.second)
                pval = &p.second;
            else
                pval->push_back(*it);
            break;
        case '&':
            if (start != it) {
                url_utf::check_fix_utf8(p.first);
                url_utf::check_fix_utf8(p.second);
                lst.push_back(std::move(p));
                // clear after move
                p.first.clear();
                p.second.clear();
            }
            pval = &p.first;
            start = it + 1; // skip '&'
            break;
        case '+':
            pval->push_back(' ');
            break;
        case '%':
            if (std::distance(it, e) > 2) {
                auto itc = it;
                unsigned char uc1 = static_cast<unsigned char>(*(++itc));
                unsigned char uc2 = static_cast<unsigned char>(*(++itc));
                if (detail::isHexChar(uc1) && detail::isHexChar(uc2)) {
                    char c = static_cast<char>((detail::HexCharToValue(uc1) << 4) + detail::HexCharToValue(uc2));
                    pval->push_back(c);
                    it = itc;
                    break;
                }
            }
            // fall thru
        default:
            pval->push_back(*it);
            break;
        }
    }
    if (start != e) {
        url_utf::check_fix_utf8(p.first);
        url_utf::check_fix_utf8(p.second);
        lst.push_back(std::move(p));
    }
    return lst;
}

template <class StrT, enable_if_str_arg_t<StrT>>
inline void url_search_params::urlencode(std::string& encoded, StrT&& value) {
    auto str_value = make_string(std::forward<StrT>(value));
    auto b = str_value.begin();
    auto e = str_value.end();

    for (auto it = b; it != e; ++it) {
        unsigned char uc = static_cast<unsigned char>(*it);
        char cenc = kEncByte[uc];
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


} // namespace whatwg

#endif // WHATWG_URL_SEARCH_PARAMS_H
