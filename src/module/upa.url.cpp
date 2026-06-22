// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
module;

#include "upa.url.h"

// Macros
#include "upa/config.h"
#include "upa/url_version.h"

#define UPA_IDNA_CPP_20
#define UPA_IDNA_CONSTEXPR_20 constexpr

module upa.url;

#if (UPA_IMPORT_STD)
import std;
#endif

#include "../idna.cpp"
#include "../url.cpp"
#include "../url_ip.cpp"
#include "../url_search_params.cpp"
#include "../url_utf.cpp"
#include "../urlpattern.cpp"
#include "../unicode_id.cpp"
#include "../public_suffix_list.cpp"
