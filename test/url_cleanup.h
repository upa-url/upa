// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef WHATWG_URL_CLEANUP_H
#define WHATWG_URL_CLEANUP_H

// For ICU cleanup
#include "url_idna.h"
#include "unicode/uclean.h"

namespace upa {

inline void url_cleanup()
{
    // ICU cleanup
    upa::IDNClose();
    u_cleanup();
}

}

#endif // WHATWG_URL_CLEANUP_H
