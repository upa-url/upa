#include "url.h"
#include "url_search_params.h"
#include "doctest-main.h"
#include "test-utils.h"
#include <map>


TEST_CASE("Various string pairs iterables") {
    pairs_list_t<std::string> output = {
        { "a", "aa" },
        { "b", "bb" }
    };

    std::map<std::string, std::string> map_pairs(output.begin(), output.end());
    std::vector<std::pair<std::string, std::string>> vec_pairs(output.begin(), output.end());

    SUBCASE("std::initializer_list") {
        whatwg::url_search_params params(output);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::map") {
        whatwg::url_search_params params(map_pairs);
        CHECK(list_eq(params, output));
    }
    SUBCASE("std::vector") {
        whatwg::url_search_params params(vec_pairs);
        CHECK(list_eq(params, output));
    }
}
