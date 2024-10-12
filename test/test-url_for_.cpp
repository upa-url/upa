// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "doctest-main.h"

#ifdef _MSC_VER
# define UPA_TEST_URL_FOR_ATL
# include "upa/url_for_atl.h"
# include <atlsimpstr.h>
# include <atlstr.h>
# include <cstringt.h>
#endif

#ifdef UPA_TEST_URL_FOR_ATL

TEST_CASE("ATL::CSimpleStringA input") {
    ATL::CStringA basestr;

    ATL::CSimpleStringA str_input("http://host/", basestr.GetManager());
    upa::url url{ str_input };
    CHECK(url.href() == "http://host/");
}

TEST_CASE("ATL::CSimpleStringW input") {
    ATL::CStringW basestr;

    ATL::CSimpleStringW str_input(L"http://host/", basestr.GetManager());
    upa::url url{ str_input };
    CHECK(url.href() == "http://host/");
}

TEST_CASE_TEMPLATE_DEFINE("ATL string input", StrT, test_url_for_atl) {
    StrT str_input("http://host/");
    upa::url url{ str_input };
    CHECK(url.href() == "http://host/");
}

TEST_CASE_TEMPLATE_INVOKE(test_url_for_atl,
    ATL::CStringA, ATL::CFixedStringT<ATL::CStringA, 16>,
    ATL::CStringW, ATL::CFixedStringT<ATL::CStringW, 16>);

#endif // UPA_TEST_URL_FOR_ATL
