// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <embedded_vhdl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

TEST_CASE("Embedded VHDL matches contrib files", "[resources]") {
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path() / "contrib";
    size_t count = 0;
    for(const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if(!entry.is_regular_file() || entry.path().extension() != ".vhdl")
            continue;
        const auto name = entry.path().lexically_relative(root).generic_string();
        INFO(name);
        std::ifstream file(entry.path(), std::ios::binary);
        REQUIRE(file.is_open());
        const std::string expected{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        REQUIRE(embedded_vhdl::get(name) == expected);
        ++count;
    }
    REQUIRE(count > 0);
}

TEST_CASE("Embedded VHDL can be read as a stream", "[resources]") {
    const auto contents = embedded_vhdl::get("ieee/std_logic_1164.vhdl");
    REQUIRE_FALSE(contents.empty());
    std::istringstream stream{std::string(contents)};
    const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    REQUIRE(text == contents);
    REQUIRE_THROWS_AS(embedded_vhdl::get("ieee/missing.vhdl"), std::out_of_range);
}
