// Copyright 2016-2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_CLEANUP_H
#define UPA_URL_CLEANUP_H

// For IDNA library cleanup
#include "upa/url_idna.h"

namespace upa {

inline void url_cleanup()
{
    upa::idna_close(true);
}

}

#endif // UPA_URL_CLEANUP_H
