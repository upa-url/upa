// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_SEARCH_PARAMS_H
#define WHATWG_URL_SEARCH_PARAMS_H

#include "url_percent_encode.h"
#include <iterator>
#include <list>
#include <string>
#include <type_traits>

namespace whatwg {


namespace {

// supported string types
template<class StrT>
struct is_char_str : std::integral_constant<bool,
    std::is_same<typename StrT::value_type, char>::value &&
    std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<typename StrT::const_iterator>::iterator_category>::value
> {};

template<size_t N>
struct is_char_str<char[N]> : std::true_type {};

template<>
struct is_char_str<char*> : std::true_type {};

// is key value pair
template <typename>
struct is_pair : std::false_type {};

template<class T1, class T2>
struct is_pair<std::pair<T1, T2>> : std::true_type {};

}


class url_search_params
{
public:
    // types
    using key_value_pair = std::pair<std::string, std::string>;
    using key_value_list = std::list<key_value_pair>;
    using const_iterator = key_value_list::const_iterator;
    using const_reverse_iterator = key_value_list::const_reverse_iterator;
    using iterator = const_iterator;
    using reverse_iterator = const_reverse_iterator;
    using value_type = key_value_pair;

    // constructors
    url_search_params();

    template<class StrT, typename std::enable_if<is_char_str<StrT>::value, int>::type = 0>
    url_search_params(const StrT& query);

    template<class ConT, typename std::enable_if<is_pair<typename ConT::value_type>::value, int>::type = 0>
    url_search_params(const ConT& cont);

    // operations
    template <class T>
    void parse(const T& query);

    template <class T, class TV>
    void append(T&& name, TV&& value);

    template <class T>
    void del(const T& name);

    template <class T>
    const std::string* get(const T& name) const;

    template <class T>
    std::list<std::string> getAll(const T& name) const;

    template <class T>
    bool has(const T& name) const;

    template <class T, class TV>
    void set(T&& name, TV&& value);

    void sort();

    void serialize(std::string& query);

    std::string to_string();

    // iterable

    const_iterator begin() const { return params_.begin(); }
    const_iterator cbegin() const { return params_.cbegin(); }
    const_iterator end() const { return params_.end(); }
    const_iterator cend() const { return params_.cend(); }
    const_reverse_iterator rbegin() const { return params_.rbegin(); }
    const_reverse_iterator crbegin() const { return params_.crbegin(); }
    const_reverse_iterator rend() const { return params_.rend(); }
    const_reverse_iterator crend() const { return params_.crend(); }

    // utils

    template <class T>
    static key_value_list do_parse(const T& query);

    template <class T>
    static void urlencode(std::string& encoded, const T& value);

private:
    key_value_list params_;
    bool is_sorted_ = false;

    static const char kEncByte[0x100];
};

// utilities

namespace {

inline const char* str_begin(const char* p) {
    return p;
}
inline const char* str_end(const char* p) {
    return p + std::char_traits<char>::length(p);
}

template <class T>
inline auto str_begin(const T& s) -> decltype(std::begin(s)) {
    return std::begin(s);
}
template <class T>
inline auto str_end(const T& s) -> decltype(std::end(s)) {
    return std::end(s);
}

}

// url_search_params inline

inline url_search_params::url_search_params()
{}

template<class StrT, typename std::enable_if<is_char_str<StrT>::value, int>::type>
inline url_search_params::url_search_params(const StrT& query)
    : params_(do_parse(query))
{}

template<class ConT, typename std::enable_if<is_pair<typename ConT::value_type>::value, int>::type>
inline url_search_params::url_search_params(const ConT& cont) {
    for (auto p : cont) {
        params_.emplace_back(p.first, p.second);
    }
}

// operations

template <class T>
inline void url_search_params::parse(const T& query) {
    params_ = do_parse(query);
    is_sorted_ = false;
}

template <class T, class TV>
inline void url_search_params::append(T&& name, TV&& value) {
    params_.emplace_back(std::forward<T>(name), std::forward<TV>(value));
    is_sorted_ = false;
}

template <class T>
inline void url_search_params::del(const T& name) {
    for (auto it = params_.begin(); it != params_.end();) {
        if (it->first == name)
            it = params_.erase(it);
        else
            ++it;
    }
}

template <class T>
inline const std::string* url_search_params::get(const T& name) const {
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == name)
            return &it->second;
    }
    return nullptr;
}

template <class T>
std::list<std::string> url_search_params::getAll(const T& name) const {
    std::list<std::string> lst;
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == name)
            lst.push_back(it->second);
    }
    return lst;
}

template <class T>
inline bool url_search_params::has(const T& name) const {
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == name)
            return true;
    }
    return false;
}

template <class T, class TV>
inline void url_search_params::set(T&& name, TV&& value) {
    bool is_match = false;
    for (auto it = params_.begin(); it != params_.end(); ) {
        if (it->first == name) {
            if (is_match) {
                it = params_.erase(it);
                continue;
            }
            it->second = std::forward<TV>(value);
            is_match = true;
        }
        ++it;
    }
    if (!is_match)
        append(std::forward<T>(name), std::forward<TV>(value));
}

inline void url_search_params::sort() {
    // https://url.spec.whatwg.org/#dom-urlsearchparams-sort
    // Sorting must be done by comparison of code units. The relative order
    // between name-value pairs with equal names must be preserved.
    if (!is_sorted_) {
        // https://en.cppreference.com/w/cpp/container/list/sort
        // std::list::sort preserves the order of equal elements.
        params_.sort([](const key_value_pair& a, const key_value_pair& b) {
            //return a.first < b.first;
            return url_utf::compare_by_code_units(
                a.first.data(), a.first.data() + a.first.size(),
                b.first.data(), b.first.data() + b.first.size()) < 0;
        });
        is_sorted_ = true;
    }
}

template <class T>
inline url_search_params::key_value_list url_search_params::do_parse(const T& query) {
    key_value_list lst;
    auto b = str_begin(query);
    auto e = str_end(query);

    // https://url.spec.whatwg.org/#dom-urlsearchparams-urlsearchparams
    if (b != e && *b == '?')
        ++b;

    key_value_pair p;
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

template <class T>
inline void url_search_params::urlencode(std::string& encoded, const T& value) {
    auto b = str_begin(value);
    auto e = str_end(value);

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

inline void url_search_params::serialize(std::string& query) {
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

inline std::string url_search_params::to_string() {
    std::string query;
    serialize(query);
    return query;
}


} // namespace whatwg

#endif // WHATWG_URL_SEARCH_PARAMS_H
