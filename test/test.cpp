#include "elaborator.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <parser.h>
#include <variant>

auto const root_path = std::filesystem::path(__FILE__).parent_path().parent_path();

void add_ieee_packages(parser::Parser& parser, std::vector<ast::design_file*>& files) {
    for(auto i : std::array<std::string, 2>{"contrib/ieee/std_logic_1164.vhdl", "contrib/ieee/numeric_bit.vhdl"}) {
        files.push_back(parser.parse_file(root_path / i, parser::encoding::UTF_8, "ieee"));
    }
}

TEST_CASE("001_minimal", "[single-file][single-instance]") {
    parser::Parser parser;
    std::vector<ast::design_file*> files;
    files.push_back(parser.parse_file(root_path / "tests/minimal/valid/basic/001_minimal.vhd", parser::encoding::UTF_8, "work"));
    add_ieee_packages(parser, files);
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
    REQUIRE(input_port->identifier == "a");
    REQUIRE(input_port->signal_mode == ast::signal_mode_e::IN);
    REQUIRE(input_port->subtype_indic != nullptr);
    REQUIRE(input_port->subtype_indic->type->name->text == "std_logic");

    auto const* output_port = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[1]);
    REQUIRE(output_port != nullptr);
    REQUIRE(output_port->identifier == "b");
    REQUIRE(output_port->signal_mode == ast::signal_mode_e::OUT);
    REQUIRE(output_port->subtype_indic != nullptr);
    REQUIRE(output_port->subtype_indic->type->name->text == "std_logic");

    auto const* arch = top_units[0]->architectures[0];
    REQUIRE(arch != nullptr);
    REQUIRE(arch->identifier == "rtl");
    REQUIRE(arch->primary != nullptr);
    REQUIRE(arch->primary->text == "top");
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

TEST_CASE("zamia_add4_minimal", "[multi-file][hierarchy]") {
    parser::Parser parser;
    std::vector<ast::design_file*> files;
    for(auto i : std::array<std::string, 3>{"tests/zamiacad/examples/add4/add4.vhdl", "tests/zamiacad/examples/add4/ha.vhdl",
                                            "tests/zamiacad/examples/add4/va.vhdl"}) {
        files.push_back(parser.parse_file(root_path / i, parser::encoding::UTF_8, "work"));
    }
    add_ieee_packages(parser, files);
    vhdl_fe::elaborator elab(parser);
    elab.add_design_files({files});
    elab.resolve_references();
    auto diags = elab.get_diagnostics();
    REQUIRE(diags.size() == 0);

    auto top_units = elab.get_top_modules();
    REQUIRE(top_units.size() == 1);
    REQUIRE(top_units[0]->identifier == "add4");
    REQUIRE(top_units[0]->port_list.size() == 5);
    REQUIRE(top_units[0]->architectures.size() == 1);

    auto const* input_a = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[0]);
    REQUIRE(input_a != nullptr);
    REQUIRE(input_a->identifier == "A");
    REQUIRE(input_a->signal_mode == ast::signal_mode_e::IN);
    REQUIRE(input_a->subtype_indic != nullptr);
    REQUIRE(input_a->subtype_indic->type->name->text == "bit_vector ( 3 downto 0 )");
    REQUIRE_FALSE(std::holds_alternative<std::monostate>(input_a->subtype_indic->type->resolved_ref));

    auto const* input_b = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[1]);
    REQUIRE(input_b != nullptr);
    REQUIRE(input_b->identifier == "B");
    REQUIRE(input_b->signal_mode == ast::signal_mode_e::IN);
    REQUIRE(input_b->subtype_indic != nullptr);
    REQUIRE(input_b->subtype_indic->type->name->text == "bit_vector ( 3 downto 0 )");
    REQUIRE_FALSE(std::holds_alternative<std::monostate>(input_b->subtype_indic->type->resolved_ref));

    auto const* carry_in = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[2]);
    REQUIRE(carry_in != nullptr);
    REQUIRE(carry_in->identifier == "C_in");
    REQUIRE(carry_in->signal_mode == ast::signal_mode_e::IN);
    REQUIRE(carry_in->subtype_indic != nullptr);
    REQUIRE(carry_in->subtype_indic->type->name->text == "bit");
    REQUIRE_FALSE(std::holds_alternative<std::monostate>(carry_in->subtype_indic->type->resolved_ref));

    auto const* sum = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[3]);
    REQUIRE(sum != nullptr);
    REQUIRE(sum->identifier == "S");
    REQUIRE(sum->signal_mode == ast::signal_mode_e::OUT);
    REQUIRE(sum->subtype_indic != nullptr);
    REQUIRE(sum->subtype_indic->type->name->text == "bit_vector ( 3 downto 0 )");
    REQUIRE_FALSE(std::holds_alternative<std::monostate>(sum->subtype_indic->type->resolved_ref));

    auto const* carry_out = std::get<ast::interface_signal_declaration*>(top_units[0]->port_list[4]);
    REQUIRE(carry_out != nullptr);
    REQUIRE(carry_out->identifier == "C");
    REQUIRE(carry_out->signal_mode == ast::signal_mode_e::OUT);
    REQUIRE(carry_out->subtype_indic != nullptr);
    REQUIRE(carry_out->subtype_indic->type->name->text == "bit");

    auto const* arch = top_units[0]->architectures[0];
    REQUIRE(arch != nullptr);
    REQUIRE(arch->identifier == "STRUCTURE");
    REQUIRE(arch->primary != nullptr);
    REQUIRE(arch->primary->text == "add4");
    REQUIRE(arch->concurrent_statements.size() == 4);

    for(std::size_t i = 0; i < arch->concurrent_statements.size(); ++i) {
        auto const* stmt = arch->concurrent_statements[i];
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->label == "va" + std::to_string(i));

        auto const* inst = std::get<ast::component_instantiation_statement*>(stmt->statement);
        REQUIRE(inst != nullptr);
        REQUIRE(inst->unit_kind == ast::instantiated_unit_kind_e::COMPONENT);
        REQUIRE(inst->unit_name == "full_adder");
        REQUIRE(inst->generic_map.empty());
        REQUIRE(inst->port_map.size() == 5);
        REQUIRE(inst->component_ref != nullptr);
        REQUIRE(inst->entity_ref != nullptr);
        REQUIRE(inst->entity_ref->identifier == "full_adder");
        REQUIRE(inst->architecture_ref != nullptr);
        REQUIRE(inst->architecture_ref->identifier == "STRUCTURE");
    }
}
