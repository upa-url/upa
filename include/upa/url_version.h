// Copyright 2023-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UPA_URL_VERSION_H
#define UPA_URL_VERSION_H

// NOLINTBEGIN(*-macro-*)

#define UPA_URL_VERSION_MAJOR 2
#define UPA_URL_VERSION_MINOR 4
#define UPA_URL_VERSION_PATCH 0

#define UPA_URL_VERSION "2.4.0"

/// @brief Encode version to one number
#define UPA_MAKE_VERSION_NUM(n1, n2, n3) ((n1) << 16 | (n2) << 8 | (n3))

/// @brief Version encoded to one number
#define UPA_URL_VERSION_NUM UPA_MAKE_VERSION_NUM( \
    UPA_URL_VERSION_MAJOR, \
    UPA_URL_VERSION_MINOR, \
    UPA_URL_VERSION_PATCH)

// NOLINTEND(*-macro-*)

#endif // UPA_URL_VERSION_H
