// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include <ast_node_factory.h>
#include <ast_nodes.h>
#include <vhdlParser/vhdlParser.h>

namespace parser {
namespace context {
ast::design_file* parse(vhdl_antlr::vhdlParser::Design_fileContext*, ast::ast_node_factory&);
ast::library_clause* parse(vhdl_antlr::vhdlParser::Library_clauseContext*, ast::ast_node_factory&);
ast::use_clause* parse(vhdl_antlr::vhdlParser::Use_clauseContext*, ast::ast_node_factory&);
ast::context_reference* parse(vhdl_antlr::vhdlParser::Context_referenceContext*, ast::ast_node_factory&);
ast::entity_declaration* parse(vhdl_antlr::vhdlParser::Entity_declarationContext*, ast::ast_node_factory&);
ast::configuration_declaration* parse(vhdl_antlr::vhdlParser::Configuration_declarationContext*, ast::ast_node_factory&);
ast::package_declaration* parse(vhdl_antlr::vhdlParser::Package_declarationContext*, ast::ast_node_factory&);
ast::package_instantiation_declaration* parse(vhdl_antlr::vhdlParser::Package_instantiation_declarationContext*, ast::ast_node_factory&);
ast::context_declaration* parse(vhdl_antlr::vhdlParser::Context_declarationContext*, ast::ast_node_factory&);
ast::architecture_body* parse(vhdl_antlr::vhdlParser::Architecture_bodyContext*, ast::ast_node_factory&);
ast::block_statement* parse(vhdl_antlr::vhdlParser::Block_statementContext*, ast::ast_node_factory&);
ast::component_instantiation_statement* parse(vhdl_antlr::vhdlParser::Component_instantiation_statementContext*, ast::ast_node_factory&);
std::variant<ast::for_generate_statement*, ast::if_generate_statement*, ast::case_generate_statement*>
parse(vhdl_antlr::vhdlParser::Generate_statementContext*, ast::ast_node_factory&);
ast::process_statement* parse(vhdl_antlr::vhdlParser::Process_statementContext*, ast::ast_node_factory&);
ast::concurrent_procedure_call_statement* parse(vhdl_antlr::vhdlParser::Concurrent_procedure_call_statementContext*,
                                                ast::ast_node_factory&);
ast::concurrent_assertion_statement* parse(vhdl_antlr::vhdlParser::Concurrent_assertion_statementContext*, ast::ast_node_factory&);
ast::concurrent_statement* parse(vhdl_antlr::vhdlParser::Concurrent_statement_with_optional_labelContext*, ast::ast_node_factory&);
ast::concurrent_statement* parse(vhdl_antlr::vhdlParser::Concurrent_statementContext*, ast::ast_node_factory&);
ast::package_body* parse(vhdl_antlr::vhdlParser::Package_bodyContext*, ast::ast_node_factory&);
std::vector<ast::signal_declaration*> parse(vhdl_antlr::vhdlParser::Signal_declarationContext*, ast::ast_node_factory&);
ast::explicit_range* parse(vhdl_antlr::vhdlParser::Explicit_rangeContext*, ast::ast_node_factory&);
ast::attribute_range* parse(vhdl_antlr::vhdlParser::Attribute_nameContext*, ast::ast_node_factory&);
ast::subtype_indication* parse(vhdl_antlr::vhdlParser::Subtype_indicationContext*, ast::ast_node_factory&);
ast::physical_unit_definition* parse(vhdl_antlr::vhdlParser::Physical_type_definitionContext*, ast::ast_node_factory&);
ast::type_definition_item parse(vhdl_antlr::vhdlParser::Type_definitionContext*, ast::ast_node_factory&);
ast::discrete_range_item parse(vhdl_antlr::vhdlParser::Discrete_rangeContext*, ast::ast_node_factory&);
ast::array_constraint* parse(vhdl_antlr::vhdlParser::Array_constraintContext*, ast::ast_node_factory&);
ast::record_constraint* parse(vhdl_antlr::vhdlParser::Record_constraintContext*, ast::ast_node_factory&);
ast::constraint_item parse(vhdl_antlr::vhdlParser::Element_constraintContext*, ast::ast_node_factory&);
ast::constraint_item parse(vhdl_antlr::vhdlParser::ConstraintContext*, ast::ast_node_factory&);
ast::resolution_item parse(vhdl_antlr::vhdlParser::Element_resolutionContext*, ast::ast_node_factory&);
ast::resolution_indication* parse(vhdl_antlr::vhdlParser::Resolution_indicationContext*, ast::ast_node_factory&);
ast::expression_item parse(vhdl_antlr::vhdlParser::ExpressionContext*, ast::ast_node_factory&);
ast::expression_item parse(vhdl_antlr::vhdlParser::ConditionContext*, ast::ast_node_factory&);
std::vector<ast::association_element*> parse(vhdl_antlr::vhdlParser::Association_listContext*, ast::ast_node_factory&);
ast::name_node* parse(vhdl_antlr::vhdlParser::NameContext*, ast::ast_node_factory&);
ast::type_mark* parse(vhdl_antlr::vhdlParser::Type_markContext*, ast::ast_node_factory&);
void parse(vhdl_antlr::vhdlParser::Generate_statement_bodyContext*, ast::generate_statement_body&, ast::ast_node_factory&);
std::vector<ast::choice_item> parse(vhdl_antlr::vhdlParser::ChoicesContext*, ast::ast_node_factory&);
void parse(vhdl_antlr::vhdlParser::Generate_statement_body_with_begin_endContext*, ast::generate_statement_body&, ast::ast_node_factory&);
std::vector<ast::interface_declaration_item> parse(vhdl_antlr::vhdlParser::Interface_elementContext*, ast::ast_node_factory&);
ast::primary_item parse(vhdl_antlr::vhdlParser::PrimaryContext*, ast::ast_node_factory&);
ast::component_declaration* parse(vhdl_antlr::vhdlParser::Component_declarationContext* ctx, ast::ast_node_factory& anf);
ast::subprogram_declaration* parse(vhdl_antlr::vhdlParser::Subprogram_declarationContext* ctx, ast::ast_node_factory& anf);
ast::subprogram_instantiation_declaration* parse(vhdl_antlr::vhdlParser::Subprogram_instantiation_declarationContext* ctx,
                                                 ast::ast_node_factory& anf);
ast::type_declaration* parse(vhdl_antlr::vhdlParser::Type_declarationContext* ctx, ast::ast_node_factory& anf);
ast::subtype_declaration* parse(vhdl_antlr::vhdlParser::Subtype_declarationContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::constant_declaration*> parse(vhdl_antlr::vhdlParser::Constant_declarationContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::interface_constant_declaration*> parse(vhdl_antlr::vhdlParser::Interface_constant_declarationContext* ctx,
                                                        ast::ast_node_factory& anf);
std::vector<ast::interface_signal_declaration*> parse(vhdl_antlr::vhdlParser::Interface_signal_declarationContext* ctx,
                                                      ast::ast_node_factory& anf);
std::vector<ast::interface_variable_declaration*> parse(vhdl_antlr::vhdlParser::Interface_variable_declarationContext* ctx,
                                                        ast::ast_node_factory& anf);
std::vector<ast::interface_file_declaration*> parse(vhdl_antlr::vhdlParser::Interface_file_declarationContext* ctx,
                                                    ast::ast_node_factory& anf);
ast::interface_type_declaration* parse(vhdl_antlr::vhdlParser::Interface_type_declarationContext* ctx, ast::ast_node_factory& anf);
ast::interface_procedure_specification* parse(vhdl_antlr::vhdlParser::Interface_procedure_specificationContext* ctx,
                                              ast::ast_node_factory& anf);
ast::interface_function_specification* parse(vhdl_antlr::vhdlParser::Interface_function_specificationContext* ctx,
                                             ast::ast_node_factory& anf);
ast::interface_subprogram_declaration* parse(vhdl_antlr::vhdlParser::Interface_subprogram_declarationContext* ctx,
                                             ast::ast_node_factory& anf);
std::vector<ast::variable_declaration*> parse(vhdl_antlr::vhdlParser::Variable_declarationContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::file_declaration*> parse(vhdl_antlr::vhdlParser::File_declarationContext* ctx, ast::ast_node_factory& anf);
ast::alias_declaration* parse(vhdl_antlr::vhdlParser::Alias_declarationContext* ctx, ast::ast_node_factory& anf);
ast::attribute_declaration* parse(vhdl_antlr::vhdlParser::Attribute_declarationContext* ctx, ast::ast_node_factory& anf);
ast::attribute_specification* parse(vhdl_antlr::vhdlParser::Attribute_specificationContext* ctx, ast::ast_node_factory& anf);
ast::group_template_declaration* parse(vhdl_antlr::vhdlParser::Group_template_declarationContext* ctx, ast::ast_node_factory& anf);
ast::group_declaration* parse(vhdl_antlr::vhdlParser::Group_declarationContext* ctx, ast::ast_node_factory& anf);
ast::subprogram_body* parse(vhdl_antlr::vhdlParser::Subprogram_bodyContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::package_body_declarative_item> parse(vhdl_antlr::vhdlParser::Process_or_package_declarative_itemContext* ctx,
                                                      ast::ast_node_factory& anf);
std::vector<ast::process_declarative_item> parse(vhdl_antlr::vhdlParser::Process_declarative_itemContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::package_declarative_item> parse(vhdl_antlr::vhdlParser::Package_declarative_itemContext* ctx, ast::ast_node_factory& anf);
ast::simple_expression* parse(vhdl_antlr::vhdlParser::Simple_expressionContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::waveform_element*> parse(vhdl_antlr::vhdlParser::WaveformContext* ctx, ast::ast_node_factory& anf);
std::vector<ast::conditional_waveform_element*> parse(vhdl_antlr::vhdlParser::Conditional_waveformsContext* ctx,
                                                      ast::ast_node_factory& anf);
std::vector<ast::selected_waveform_element*> parse(vhdl_antlr::vhdlParser::Selected_waveformsContext* ctx, ast::ast_node_factory& anf);
ast::concurrent_signal_assignment_any* parse(vhdl_antlr::vhdlParser::Concurrent_signal_assignment_anyContext* any,
                                             ast::ast_node_factory& anf);
ast::concurrent_selected_signal_assignment* parse(vhdl_antlr::vhdlParser::Concurrent_selected_signal_assignmentContext* selected,
                                                  ast::ast_node_factory& anf);
std::vector<ast::sequential_statement_item> parse(vhdl_antlr::vhdlParser::Sequence_of_statementsContext* ctx, ast::ast_node_factory& anf);

} // namespace context
} // namespace parser
