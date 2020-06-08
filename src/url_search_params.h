// Copyright 2016-2019 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_SEARCH_PARAMS_H
#define WHATWG_URL_SEARCH_PARAMS_H

#include <list>
#include <string>
#include <type_traits>
#include "str_arg.h"
#include "url_percent_encode.h"

namespace whatwg {


namespace {

// is key value pair
template <typename>
struct is_pair : std::false_type {};

template<class T1, class T2>
struct is_pair<std::pair<T1, T2>> : std::true_type {};

}


class url;

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
    using size_type = key_value_list::size_type;
    using value_type = key_value_pair;

    // constructors
    url_search_params();

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    url_search_params(Args&&... query);

    template<class ConT, typename std::enable_if<is_pair<typename ConT::value_type>::value, int>::type = 0>
    url_search_params(const ConT& cont);

    // operations
    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    void parse(Args&&... query);

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

    // capacity

    bool empty() const noexcept { return params_.empty(); }
    size_type size() const noexcept { return params_.size(); }

    // utils

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    static key_value_list do_parse(Args&&... query);

    template <class ...Args, enable_if_str_arg_t<Args...> = 0>
    static void urlencode(std::string& encoded, Args&&... value);

private:
    void update();

private:
    key_value_list params_;
    bool is_sorted_ = false;
    url* url_ptr_ = nullptr;

    static const char kEncByte[0x100];
};


// url_search_params inline

inline url_search_params::url_search_params()
{}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline url_search_params::url_search_params(Args&&... query)
    : params_(do_parse(std::forward<Args>(query)...))
{}

template<class ConT, typename std::enable_if<is_pair<typename ConT::value_type>::value, int>::type>
inline url_search_params::url_search_params(const ConT& cont) {
    for (auto p : cont) {
        params_.emplace_back(make_string(p.first), make_string(p.second));
    }
}

// operations

template <class ...Args, enable_if_str_arg_t<Args...>>
inline void url_search_params::parse(Args&&... query) {
    params_ = do_parse(std::forward<Args>(query)...);
    is_sorted_ = false;
}

template <class T, class TV>
inline void url_search_params::append(T&& name, TV&& value) {
    params_.emplace_back(
        make_string(std::forward<T>(name)),
        make_string(std::forward<TV>(value))
    );
    is_sorted_ = false;
    update();
}

template <class T>
inline void url_search_params::del(const T& name) {
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end();) {
        if (it->first == str_name)
            it = params_.erase(it);
        else
            ++it;
    }
    update();
}

template <class T>
inline const std::string* url_search_params::get(const T& name) const {
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
            return &it->second;
    }
    return nullptr;
}

template <class T>
std::list<std::string> url_search_params::getAll(const T& name) const {
    std::list<std::string> lst;
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
            lst.push_back(it->second);
    }
    return lst;
}

template <class T>
inline bool url_search_params::has(const T& name) const {
    const auto str_name = make_string(name);
    for (auto it = params_.begin(); it != params_.end(); ++it) {
        if (it->first == str_name)
            return true;
    }
    return false;
}

template <class T, class TV>
inline void url_search_params::set(T&& name, TV&& value) {
    auto str_name = make_string(std::forward<T>(name));
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
        params_.sort([](const key_value_pair& a, const key_value_pair& b) {
            //return a.first < b.first;
            return url_utf::compare_by_code_units(
                a.first.data(), a.first.data() + a.first.size(),
                b.first.data(), b.first.data() + b.first.size()) < 0;
        });
        is_sorted_ = true;
    }
    update();
}

template <class ...Args, enable_if_str_arg_t<Args...>>
inline url_search_params::key_value_list url_search_params::do_parse(Args&&... query) {
    key_value_list lst;

    auto str_query = make_string(std::forward<Args>(query)...);
    auto b = str_query.begin();
    auto e = str_query.end();

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

template <class ...Args, enable_if_str_arg_t<Args...>>
inline void url_search_params::urlencode(std::string& encoded, Args&&... value) {
    auto str_value = make_string(std::forward<Args>(value)...);
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
