// SPDX-License-Identifier: MIT
#pragma once

#include <string_view>
#include <vector>

namespace embedded_vhdl {
struct resource {
    std::string_view name;
    std::string_view contents;
};

const std::vector<resource>& all();

// Names are relative to contrib, with '/' separators (e.g. ieee/numeric_bit.vhdl).
// Returns the original file bytes with static lifetime. Throws std::out_of_range
// for unknown names. No filesystem access is performed.
std::string_view get(std::string_view name);
} // namespace embedded_vhdl
