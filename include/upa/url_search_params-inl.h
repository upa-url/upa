// Copyright 2016-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
// This file should be included at the end of url.h.
// It needs declarations of url, url_search_params and
// url_search_params_ptr classes
//

#ifndef UPA_URL_SEARCH_PARAMS_INL_H
#define UPA_URL_SEARCH_PARAMS_INL_H

#include <memory> // std::addressof


namespace upa {

// url_search_params class

inline url_search_params::url_search_params(url* url_ptr)
    : params_(do_parse(false, url_ptr->get_part_view(url::QUERY)))
    , url_ptr_(url_ptr)
{}

inline void url_search_params::update() {
    if (url_ptr_ && url_ptr_->is_valid()) {
        detail::url_setter urls(*url_ptr_);

        if (empty()) {
            // set query to null
            urls.clear_part(url::QUERY);
            urls.potentially_strip_trailing_spaces_from_an_opaque_path();
        } else {
            std::string& str_query = urls.start_part(url::QUERY);
            serialize(str_query);
            urls.save_part();
            urls.set_flag(url::QUERY_FLAG); // not null query
        }
    }
}

namespace detail {

 // url_search_params_ptr class

inline url_search_params_ptr& url_search_params_ptr::operator=(const url_search_params_ptr& other) {
    if (ptr_ && this != std::addressof(other)) {
        if (other.ptr_) {
            ptr_->copy_params(*other.ptr_);
        } else {
            assert(ptr_->url_ptr_);
            ptr_->parse_params(ptr_->url_ptr_->get_part_view(url::QUERY));
        }
    }
    return *this;
}


} // namespace detail
} // namespace upa

#endif // UPA_URL_SEARCH_PARAMS_INL_H
