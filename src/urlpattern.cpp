// Copyright 2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/urlpattern.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace upa {
namespace {

inline constexpr std::pair<std::string_view, std::optional<std::string> urlpattern_init::*>
component_map[] = {
    {"protocol", &urlpattern_init::protocol},
    {"username", &urlpattern_init::username},
    {"password", &urlpattern_init::password},
    {"hostname", &urlpattern_init::hostname},
    {"port", &urlpattern_init::port},
    {"pathname", &urlpattern_init::pathname},
    {"search", &urlpattern_init::search},
    {"hash", &urlpattern_init::hash},
    {"baseURL", &urlpattern_init::base_url},
};

constexpr std::uint8_t component_hash(std::string_view sv) {
    assert(sv.length() >= 4);
    return (static_cast<unsigned char>(sv[0]) +
        static_cast<unsigned char>(sv[1]) +
        static_cast<unsigned char>(sv[2]) * 3) & 0xF;
}

constexpr auto build_index() {
    std::array<std::uint8_t, 16> table{};
    for (auto& item : table)
        item = 0xFF;
    for (std::size_t i = 0; i < sizeof(component_map) / sizeof(component_map[0]); ++i)
        table[component_hash(component_map[i].first)] = static_cast<std::uint8_t>(i);
    return table;
}

inline constexpr auto component_index = build_index();

} // namespace


std::optional<std::string> urlpattern_init::* urlpattern_init::get_member(std::string_view name) {
    if (name.length() < 4 || name.length() > 8)
        return nullptr;
    const auto ind = component_index[component_hash(name)];
    if (ind == 0xFF)
        return nullptr;
    const auto& [member_name, member_ptr] = component_map[ind];
    if (member_name != name)
        return nullptr;
    return member_ptr;
}


} // namespace upa
