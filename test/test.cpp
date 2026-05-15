#include "elaborator.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <parser.h>

TEST_CASE("001_minimal", "[single-file][single-instance]") {
    parser::Parser parser;
    auto const root_path = std::filesystem::path(__FILE__).parent_path().parent_path();
    std::vector<ast::design_file*> files;
    for(auto i : std::array<std::string, 2>{"contrib/ieee/std_logic_1164.vhdl", "contrib/ieee/numeric_bit.vhdl"}) {
        files.push_back(parser.parse_file(root_path / i, parser::encoding::UTF_8, "ieee"));
    }
    files.push_back(parser.parse_file(root_path / "tests/minimal/valid/basic/001_minimal.vhd", parser::encoding::UTF_8, "work"));
    vhdl_fe::elaborator elab(parser);
    elab.add_design_files({files});
    elab.resolve_references();
    auto diags = elab.get_diagnostics();
    REQUIRE(diags.size() == 0);
    auto top_units = elab.get_top_modules();
    REQUIRE(top_units.size() == 1);
    REQUIRE(top_units[0]->identifier == "top");
    REQUIRE(top_units[0]->port_list.size() == 2);
    REQUIRE(top_units[0]->architectures.size() == 1);

    auto const* input_port = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[0]);
    REQUIRE(input_port != nullptr);
    REQUIRE(input_port->identifier_list.size() == 1);
    REQUIRE(input_port->identifier_list[0] == "a");
    REQUIRE(input_port->signal_mode == ast::signal_mode_e::IN);
    REQUIRE(input_port->subtype_indic != nullptr);
    REQUIRE(input_port->subtype_indic->type == "std_logic");

    auto const* output_port = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[1]);
    REQUIRE(output_port != nullptr);
    REQUIRE(output_port->identifier_list.size() == 1);
    REQUIRE(output_port->identifier_list[0] == "b");
    REQUIRE(output_port->signal_mode == ast::signal_mode_e::OUT);
    REQUIRE(output_port->subtype_indic != nullptr);
    REQUIRE(output_port->subtype_indic->type == "std_logic");

    auto const* arch = top_units[0]->architectures[0];
    REQUIRE(arch != nullptr);
    REQUIRE(arch->identifier == "rtl");
    REQUIRE(arch->primary == "top");
    REQUIRE(arch->block_declarative_items.empty());
    REQUIRE(arch->concurrent_statements.size() == 1);

    auto const* concurrent_statement = arch->concurrent_statements[0];
    REQUIRE(concurrent_statement != nullptr);
    REQUIRE(concurrent_statement->label.empty());

    auto const* assignment = std::get<ast::concurrent_signal_assignment_any*>(concurrent_statement->statement);
    REQUIRE(assignment != nullptr);
    REQUIRE(assignment->kind == ast::concurrent_signal_assignment_kind_e::SIMPLE);
    REQUIRE_FALSE(assignment->guarded);
    REQUIRE(assignment->waveform.size() == 1);

    auto const* target = std::get<ast::name_node*>(assignment->target);
    REQUIRE(target != nullptr);
    REQUIRE(target->text == "b");

    auto const* waveform = assignment->waveform[0];
    REQUIRE(waveform != nullptr);
    auto const* value = std::get<ast::simple_expression*>(waveform->value);
    REQUIRE(value != nullptr);
    REQUIRE(value->kind == ast::simple_expression_kind_e::PRIMARY);

    auto const* source = std::get<ast::name_node*>(value->first_primary);
    REQUIRE(source != nullptr);
    REQUIRE(source->text == "a");
}
