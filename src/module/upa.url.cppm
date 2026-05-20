// Copyright 2026 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
module;

#include "upa.url.h"

export module upa.url;

#if (UPA_IMPORT_STD)
import std;
#endif

#include "upa/idna.h"
#include "upa/url.h"
#include "upa/urlpattern.h"
#include "upa/public_suffix_list.h"
