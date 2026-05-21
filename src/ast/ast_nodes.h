// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include "ast_nodes_fwd.h"
#include <string>
#include <variant>
#include <vector>

namespace ast {

struct ast_node_factory;

enum class direction_e { TO, DOWNTO };

enum class file_open_mode_e { UNDEF, IN, OUT };

enum class protected_type_e { DECL, BODY };

enum class entity_aspect_e { ENTITY, CONFIGURATION, OPEN };

enum class signal_mode_e { NONE, IN, OUT, INOUT, BUFFER, LINKAGE };

enum class expression_kind_e { CONDITIONAL_PRIMARY, SIMPLE, SHIFT, RELATIONAL, LOGICAL, RAW };

enum class simple_expression_kind_e { PRIMARY, POWER, PREFIX, MUL_DIV, ADD_SUB, RAW };

enum class operation_kind_e {
    NONE,
    PLUS,
    MINUS,
    AMPERSAND,
    MUL,
    DIV,
    MOD,
    REM,
    DOUBLESTAR,
    ABS,
    NOT,
    SIGN_PLUS,
    SIGN_MINUS,
    AND,
    OR,
    NAND,
    NOR,
    XOR,
    XNOR,
    SLL,
    SRL,
    SLA,
    SRA,
    ROL,
    ROR,
    EQ,
    NE,
    LT,
    CONASGN,
    GT,
    GE,
    EQ_MATCH,
    NE_MATCH,
    LT_MATCH,
    LE_MATCH,
    GT_MATCH,
    GE_MATCH
};

enum class instantiated_unit_kind_e { COMPONENT, ENTITY, CONFIGURATION };

enum class concurrent_signal_assignment_kind_e { SIMPLE, CONDITIONAL, SELECTED };

enum class entity_class_e {
    ENTITY,
    ARCHITECTURE,
    CONFIGURATION,
    PROCEDURE,
    FUNCTION,
    PACKAGE,
    TYPE,
    SUBTYPE,
    CONSTANT,
    SIGNAL,
    VARIABLE,
    COMPONENT,
    LABEL,
    LITERAL,
    UNITS,
    GROUP,
    FILE,
    PROPERTY,
    SEQUENCE
};

enum class delay_mechanism_e { TRANSPORT, INERTIAL };

enum class physical_unit_lieral_e { NONE, DECIMAL, BASED };

enum class generic_map_aspect_e { MAP, BOX, DEFAULT };

enum class force_mode_e { IN, OUT };
enum class name_kind_e { SIMPLE, SELECTED, SLICE, ATTRIBUTE, CALL, EXTERNAL };
// clang-format off
using declaration_ref = std::variant<
    std::monostate,
    builtin_declaration*,
    entity_declaration*,
    architecture_body*,
    configuration_declaration*,
    package_declaration*,
    package_body*,
    package_instantiation_declaration*,
    context_declaration*,
    component_declaration*,
    signal_declaration*,
    subprogram_declaration*,
    subprogram_declaration_overload*,
    subprogram_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    group_template_declaration*,
    group_declaration*,
    interface_constant_declaration*,
    interface_signal_declaration*,
    interface_variable_declaration*,
    interface_file_declaration*,
    interface_type_declaration*,
    interface_subprogram_declaration*,
    interface_package_declaration*>;

using primary_item = std::variant<
    literal_node*,
    name_node*,
    allocator*, 
    aggregate*, 
    qualified_expression*>;

using expression_item = std::variant<
    simple_expression*,
    conditional_primary*,
    literal_node*, 
    allocator*, 
    aggregate*, 
    qualified_expression*,
    binary_expression*>;

using loop_expression_item = std::variant<
    simple_expression*,
    conditional_primary*,
    literal_node*, 
    allocator*, 
    aggregate*, 
    qualified_expression*,
    binary_expression*, parameter_specification*>;

using block_declarative_item = std::variant<
    signal_declaration*,
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    package_declaration*,
    package_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    attribute_specification*,
    use_clause*,
    group_template_declaration*,
    group_declaration*,
    subprogram_body*,
    package_body*,
    disconnection_specification*,
    component_declaration*,
    configuration_specification*>;

using concurrent_statement_item =  std::variant<
    block_statement*,
    component_instantiation_statement*,
    // generate_statement
    for_generate_statement*,
    if_generate_statement*,
    case_generate_statement*,
    // concurrent_statement_with_optional_label
    process_statement*,
    concurrent_procedure_call_statement*,
    concurrent_assertion_statement*,
    //concurrent_signal_assignment_statement
    concurrent_signal_assignment_any*,
    concurrent_selected_signal_assignment*>;

using interface_declaration_item = std::variant<
      //interface_object_declaration
    interface_constant_declaration*,
    interface_signal_declaration*,
    interface_variable_declaration*,
    interface_file_declaration*,
    interface_type_declaration*,
    interface_subprogram_declaration*,
    interface_package_declaration*>;

using package_declarative_item = std::variant<
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    package_declaration*,
    package_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    attribute_specification*,
    use_clause*,
    group_template_declaration*,
    group_declaration*,
    signal_declaration*,
    component_declaration*,
    disconnection_specification*>;

using package_body_declarative_item = std::variant<
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    package_declaration*,
    package_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    attribute_specification*,
    use_clause*,
    group_template_declaration*,
    group_declaration*,
    subprogram_body*,
    package_body*>;

using entity_declarative_item = std::variant<
    signal_declaration*,
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    package_declaration*,
    package_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    attribute_specification*,
    use_clause*,
    group_template_declaration*,
    group_declaration*,
    subprogram_body*,
    package_body*,
    disconnection_specification*>;

using unit_item = std::variant<
    library_clause*,
    use_clause*,
    context_reference*,
    entity_declaration*,
    configuration_declaration*, 
    package_declaration*,
    package_instantiation_declaration*, 
    context_declaration*, 
    architecture_body*, 
    package_body*>;

using type_definition_item = std::variant<
    // scalar_type_definition
    numeric_type_definition*,
    enumeration_type_definition*,
    // composite_type_definition
    // array_type_definition*,
    unbounded_array_definition *,
    constrained_array_definition*,
    record_type_definition*,
    // access_type_definition
    subtype_indication*,
    // file_type_definition
    type_mark*,
    // protected_type_definition
    protected_type_definition*,
    protected_type_declaration*>;
using protected_type_declarative_item = std::variant<
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    attribute_specification*,
    use_clause*>;

using process_declarative_item = std::variant<
    // process_or_package_declarative_item
    subprogram_declaration*,
    subprogram_instantiation_declaration*,
    package_declaration*,
    package_instantiation_declaration*,
    type_declaration*,
    subtype_declaration*,
    constant_declaration*,
    variable_declaration*,
    file_declaration*,
    alias_declaration*,
    attribute_declaration*,
    attribute_specification*,
    use_clause*,
    group_template_declaration*,
    group_declaration*,
    subprogram_body*,
    package_body*>;

using constraint_item = std::variant< 
    // range_constraint
    explicit_range*, attribute_range*,
    // element_constraint
    array_constraint*, record_constraint*>;

using discrete_range_item = std::variant<
    subtype_indication*,
    // range
    attribute_range*,
    explicit_range*>;
    

using resolution_item = std::variant<
    resolution_indication*,
    record_resolution*>;
using configuration_declarative_item = std::variant<
    use_clause*,
    attribute_specification*,
    group_declaration*>;
using configuration_item = std::variant<
    block_configuration*,
    component_configuration*>;
using generate_specification_item= std::variant<
    // discrete_range
    subtype_indication*,
    attribute_range*,
    explicit_range*,
    //expression
    simple_expression*,
    conditional_primary*,
    literal_node*, 
    allocator*, 
    aggregate*, 
    qualified_expression*,
    binary_expression*>;
using context_item = std::variant<
    library_clause*,
    use_clause*,
    context_reference*>;
using choice_item = std::variant<
    // discrete_range
    subtype_indication*,
    // range
    attribute_range*,
    explicit_range*,
    simple_expression*,
    literal_node*>;
using sequential_statement_item = std::variant<
    wait_statement*,
    assertion_statement*,
    report_statement*,
    // signal_assignment_statement
    //. simple_signal_assignment
    simple_waveform_assignment*,
    simple_force_assignment*,
    simple_release_assignment*,
    //. conditional_signal_assignment
    conditional_waveform_assignment*,
    conditional_force_assignment*,
    //. selected_signal_assignment
    selected_waveform_assignment*,
    selected_force_assignment*,
    // variable_assignment_statement
    simple_variable_assignment*,
    conditional_variable_assignment*,
    selected_variable_assignment*,

    procedure_call_statement*,
    if_statement*,
    case_statement*,
    loop_statement*,
    next_statement*,
    exit_statement*,
    return_statement*,
    null_statement*>;
using target_item = std::variant<
    aggregate*,
    name_node*>;

// clang-format on

struct literal_node {
    std::string text;
    declaration_ref resolved_ref;
};

struct name_slice {
    explicit_range* range{nullptr};
};

struct name_attribute {
    std::string signature;
    std::string designator;
};

struct name_arguments {
    std::vector<association_element*> associations;
};

struct name_node {
    // std::string text;
    std::string value;
    name_kind_e kind{name_kind_e::SIMPLE};
    name_node* prefix{nullptr};
    name_slice* slice{nullptr};
    name_attribute* attribute{nullptr};
    name_arguments* arguments{nullptr};
    // elaborated members
    declaration_ref resolved_ref;
};

struct type_mark {
    name_node* name{nullptr};
    // elaborated members
    declaration_ref resolved_ref;
};

struct builtin_declaration {
    std::string identifier;
    entity_class_e entity_class;
};

struct library_clause {
    std::vector<std::string> names;
};

struct used_package {
    std::string identifier;
    std::vector<std::string> suffixes;
    // elaborated members
    package_declaration* package_ref{nullptr};
    declaration_ref selected_ref;
};

struct use_clause {
    std::vector<used_package*> clauses;
};

struct context_reference {
    std::vector<selected_name> selected_names;
};

struct array_constraint {
    std::vector<discrete_range_item> index_constraint;
    bool is_open;
};

struct record_element_constraint {
    std::string identifier;
    subtype_indication* indication;
};

struct record_constraint {
    std::vector<record_element_constraint> constraint;
};

struct signal_declaration {
    std::string identifier;
    signal_mode_e mode{ast::signal_mode_e::NONE};
    resolution_indication* resolution;
    type_mark* type;
    constraint_item constraint;
    bool is_bus;
    // elaborated members
    declaration_ref type_ref;
};

struct block_specification {
    std::string label;
    generate_specification_item generate_specification;
};

struct component_configuration {
    component_specification* component_spec;
    binding_indication* binding;
    block_configuration* block_config;
};

struct block_configuration {
    block_specification* block_spec;
    std::vector<use_clause*> use_clauses;
    std::vector<configuration_item> configuration_items;
};

struct configuration_declaration {
    std::string identifier;
    std::string name;
    std::vector<configuration_declarative_item> declarative_items;
    block_configuration* block_config;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    // elaborated members
    entity_declaration* entity_ref{nullptr};
};

struct component_specification {
    std::vector<std::string> instantiations;
    std::string name;
    // elaborated members
    component_declaration* component_ref{nullptr};
};

struct binding_indication {
    entity_aspect_e type{entity_aspect_e::OPEN};
    std::string unit_ref;
    std::string identifier;
    std::vector<association_element*> generic_map;
    std::vector<association_element*> port_map;
    // elaborated members
    entity_declaration* entity_ref{nullptr};
    architecture_body* architecture_ref{nullptr};
    configuration_declaration* configuration_ref{nullptr};
};

struct configuration_specification {
    component_specification* component_spec;
    binding_indication* binding;
};

struct package_declaration {
    std::string identifier;
    std::vector<interface_declaration_item> generic_list;
    std::vector<association_element*> generic_map;
    std::vector<package_declarative_item> declarations;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    std::vector<package_body*> body;
};

struct package_instantiation_declaration {
    std::string identifier;
    name_node* target_name{nullptr};
    std::vector<association_element*> generic_map;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    package_declaration* target_package_ref{nullptr};
};

struct selected_name {
    std::string identifier;
    std::string suffix;
    // elaborated members
    declaration_ref resolved_ref;
};

struct context_declaration {
    std::string identifier;
    std::vector<context_item> context_items;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
};

struct component_declaration {
    std::string identifier;
    std::vector<interface_declaration_item> generic_list;
    std::vector<interface_declaration_item> port_list;
};

struct subprogram_declaration {
    bool is_function{false};
    std::string designator;
    std::vector<interface_declaration_item> generic_list;
    std::vector<association_element*> generic_map;
    std::vector<interface_declaration_item> parameter_list;
    type_mark* return_type;
    // elaborated members
    declaration_ref return_type_ref;
};

struct subprogram_declaration_overload {
    std::string designator;
    std::vector<subprogram_declaration*> overloads;
};

struct subprogram_instantiation_declaration {
    std::string designator;
    name_node* target_name{nullptr};
    // elaborated members
    subprogram_declaration* target_ref{nullptr};
};

struct enumeration_type_definition {
    std::vector<std::string> enumeration_literals;
};

struct secondary_unit {
    physical_unit_lieral_e physical_literal_type{physical_unit_lieral_e::NONE};
    name_node* physical_literal_name{nullptr};
};

struct physical_unit_definition {
    std::string primary_unit;
    std::vector<secondary_unit*> secondary_unit_identifiers;
};

struct numeric_type_definition {
    std::variant<explicit_range*, attribute_range*> range;
    physical_unit_definition* physical_unit;
};

struct unbounded_array_definition {
    std::vector<type_mark*> index_subtype_definitions;
};

struct constrained_array_definition {
    std::vector<discrete_range_item> index_constraints;
    subtype_indication* subtype_indication_;
};

struct element_declaration {
    std::string identifier;
    subtype_indication* element_subtype_definition;
};

struct record_type_definition {
    std::vector<element_declaration*> element_declarations;
    std::string identifier;
};

struct type_declaration {
    std::string identifier;
    type_definition_item type;
};

struct protected_type_definition {
    protected_type_e type;
    std::vector<protected_type_declarative_item> declarations;
    std::vector<process_declarative_item> body_declarations;
};

struct protected_type_declaration {
    std::vector<protected_type_declarative_item> protected_type_declarative_items;
    std::string identifier;
};

struct explicit_range {
    expression_item left;
    expression_item right;
    direction_e direction;
};

struct attribute_range {
    std::string name;
    std::string attribute_designator;
    // elaborated members
    declaration_ref prefix_ref;
};

struct subtype_declaration {
    std::string identifier;
    subtype_indication* indication;
};

struct constant_declaration {
    std::string identifier;
    subtype_indication* indication;
    expression_item expr;
};

struct variable_declaration {
    bool shared;
    std::string identifier;
    subtype_indication* indication;
    expression_item expr;
};

struct file_declaration {
    std::string identifier;
    subtype_indication* indication;
    // file_open_information
    expression_item open_expression;
    file_open_mode_e dir{file_open_mode_e::UNDEF};
    expression_item file_logical_name;
};

struct alias_declaration {
    std::string alias_designator;
    subtype_indication* indication;
    std::string name;
    // signature
    std::vector<type_mark*> type_marks;
    type_mark* return_type_mark;
    // elaborated members
    declaration_ref name_ref;
    std::vector<declaration_ref> type_mark_refs;
    declaration_ref return_type_mark_ref;
};

struct attribute_declaration {
    std::string identifier;
    type_mark* type;
    // elaborated members
    declaration_ref type_ref;
};

struct interface_constant_declaration {
    std::string identifier;
    bool is_in{false};
    subtype_indication* subtype_indic;
    expression_item expression;
};

struct interface_signal_declaration {
    std::string identifier;
    signal_mode_e signal_mode{signal_mode_e::NONE};
    subtype_indication* subtype_indic;
    bool is_bus{false};
    expression_item expression;
};

struct interface_variable_declaration {
    std::string identifier;
    signal_mode_e signal_mode{signal_mode_e::NONE};
    subtype_indication* subtype_indic;
    expression_item expression;
};

struct interface_type_declaration {
    std::string identifier;
};

struct interface_file_declaration {
    std::string identifier;
    subtype_indication* subtype_indic;
};

struct interface_procedure_specification {
    std::string designator;
    bool is_parameter{false};
    std::vector<interface_declaration_item> formal_parameter_list;
};

struct interface_function_specification {
    bool is_pure{false};
    bool is_impure{false};
    std::string designator;
    std::vector<interface_declaration_item> formal_parameter_list;
    type_mark* return_type_mark;
    // elaborated members
    declaration_ref return_type_ref;
};

struct interface_subprogram_declaration {
    std::variant<interface_procedure_specification*, interface_function_specification*> nterface_subprogram_specification;
    bool is_box{false};
    name_node* interface_subprogram_default{nullptr};
};

struct interface_package_declaration {
    std::string identifier;
    name_node* name{nullptr};
    generic_map_aspect_e map_type{generic_map_aspect_e::MAP};
    std::vector<association_element*> generic_map_aspect;
    // elaborated members
    package_declaration* package_ref{nullptr};
};

struct attribute_specification {
    std::string attribute_designator;
    std::vector<std::string> entity_name_list;
    entity_class_e entity_class;
    expression_item expr;
    // elaborated members
    std::vector<declaration_ref> entity_refs;
};

struct entity_class_entry {
    entity_class_e entity_class;
    bool is_box;
};

struct group_template_declaration {
    std::string identifier;
    std::vector<entity_class_entry*> entity_class_entry_list;
};

struct group_declaration {
    std::string identifier;
    std::string name;
    std::vector<std::string> group_constituent_list;
    // elaborated members
    declaration_ref template_ref;
    std::vector<declaration_ref> constituent_refs;
};

struct subprogram_body {
    subprogram_declaration* specification{nullptr};
    std::vector<package_body_declarative_item> declarative_items;
    std::vector<sequential_statement*> sequential_statements;
};

struct package_body {
    std::string identifier;
    std::vector<package_body_declarative_item> declarative_items;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    // elaborated members
    package_declaration* package_ref{nullptr};
};

struct disconnection_specification {
    std::string signal_name;
    type_mark* type;
    expression_item after_expression;
    // elaborated members
    declaration_ref signal_ref;
    declaration_ref type_ref;
};

struct actual_designator {
    bool is_inertial;
    bool is_open;
    expression_item expr;
    subtype_indication* indication;
};

struct association_element {
    std::string formal_name;
    std::string formal_paren_name;
    std::string actual_name;
    actual_designator* act_designator;
    // elaborated members
    declaration_ref formal_ref;
    declaration_ref actual_ref;
};

struct record_element_resolution {
    std::string identifier;
    resolution_indication* resolution;
};

struct record_resolution {
    std::vector<record_element_resolution*> elems;
};

struct resolution_indication {
    std::string name;
    resolution_item elem_resolution;
    // elaborated members
    declaration_ref resolution_ref;
};

struct subtype_indication {
    resolution_indication* resolution{nullptr};
    type_mark* type{nullptr};
    constraint_item constr;
};

struct qualified_expression {
    type_mark* type{nullptr};
    aggregate* aggr;
    // elaborated members
    declaration_ref type_ref;
};

struct allocator {
    subtype_indication* si;
    qualified_expression* qe;
};

struct aggregate {
    std::vector<element_association*> elem_assoc;
};

struct element_association {
    std::vector<simple_expression*> choices;
    expression_item expr;
};

struct simple_expression {
    simple_expression_kind_e kind{simple_expression_kind_e::RAW};
    operation_kind_e op{operation_kind_e::NONE};
    primary_item first_primary;
    primary_item secondary_primary;
    simple_expression* lhs{nullptr};
    simple_expression* rhs{nullptr};
    simple_expression* operand{nullptr};
};

struct conditional_primary {
    primary_item prim;
};

struct binary_expression {
    operation_kind_e op;
    expression_item lhs;
    expression_item rhs;
};

struct concurrent_statement_body {
    std::vector<block_declarative_item*> block_declarative_items;
    std::vector<concurrent_statement*> concurrent_statements;
    std::string label;
};

struct block_statement {
    expression_item condition;
    std::vector<interface_declaration_item> generic_list;
    std::vector<association_element*> generic_map;
    std::vector<interface_declaration_item> port_list;
    std::vector<association_element*> port_map;
    std::vector<block_declarative_item> block_declarative_items;
    std::vector<concurrent_statement*> concurrent_statements;
    std::string label;
};

struct concurrent_statement {
    std::string label;
    concurrent_statement_item statement;
};

struct component_instantiation_statement {
    std::string label;
    instantiated_unit_kind_e unit_kind{instantiated_unit_kind_e::COMPONENT};
    std::string unit_name;
    std::string architecture_id;
    std::vector<association_element*> generic_map;
    std::vector<association_element*> port_map;
    // elaborated members
    component_declaration* component_ref{nullptr};
    entity_declaration* entity_ref{nullptr};
    architecture_body* architecture_ref{nullptr};
    configuration_declaration* configuration_ref{nullptr};
    component_instantiation_statement* clone(ast_node_factory& anf);
};

struct generate_statement_body {
    std::vector<block_declarative_item> block_declarative_items;
    std::vector<concurrent_statement*> concurrent_statements;
};

struct for_generate_statement {
    std::string label;
    std::string parameter;
    std::string range;
    generate_statement_body body;
};

struct if_generate_clause {
    std::string label;
    expression_item condition;
    generate_statement_body body;
};

struct if_generate_statement {
    std::string label;
    std::vector<if_generate_clause*> clauses;
    bool has_else{false};
    std::string else_label;
    generate_statement_body else_body;
};

struct case_generate_alternative {
    std::string label;
    std::vector<choice_item> choices;
    generate_statement_body body;
};

struct case_generate_statement {
    std::string label;
    std::string expression;
    std::vector<case_generate_alternative*> alternatives;
};

struct process_statement {
    std::string label;
    bool postponed{false};
    bool sensitivity_all{false};
    std::vector<std::string> sensitivity_list;
    std::vector<process_declarative_item> declarative_items;
    std::vector<sequential_statement*> sequential_statements;
    // elaborated members
    std::vector<declaration_ref> sensitivity_refs;
};

struct concurrent_procedure_call_statement {
    std::string label;
    bool postponed{false};
    std::string name;
    // elaborated members
    declaration_ref procedure_ref;
};

struct concurrent_assertion_statement {
    std::string label;
    bool postponed{false};
    expression_item condition;
    expression_item report;
    expression_item severity;
};

struct waveform_element {
    expression_item value;
    expression_item after_expression;
};

struct conditional_waveform_element {
    std::vector<waveform_element*> waveform;
    expression_item condition;
};

struct concurrent_signal_assignment_any {
    bool postponed{false};
    concurrent_signal_assignment_kind_e kind{concurrent_signal_assignment_kind_e::SIMPLE};
    target_item target;
    bool guarded{false};
    delay_mechanism_e delay_mechanism_type{delay_mechanism_e::TRANSPORT};
    expression_item delay_mechanism_reject;
    std::vector<waveform_element*> waveform;
    std::vector<conditional_waveform_element*> conditional_waveforms;
};

struct selected_waveform_element {
    std::vector<waveform_element*> waveform;
    std::vector<choice_item> choices;
};

struct concurrent_selected_signal_assignment {
    bool postponed{false};
    expression_item with_expression;
    target_item target;
    bool guarded{false};
    delay_mechanism_e delay_mechanism_type{delay_mechanism_e::TRANSPORT};
    expression_item delay_mechanism_reject;
    std::vector<selected_waveform_element*> selected_waveforms;
};

struct entity_statement {
    std::string label;
    std::variant<concurrent_assertion_statement*, concurrent_procedure_call_statement*, process_statement*> statement;
};

struct entity_declaration {
    std::string identifier;
    std::vector<interface_declaration_item> generic_list;
    std::vector<interface_declaration_item> port_list;
    std::vector<entity_declarative_item> entity_declarative_items;
    std::vector<entity_statement*> entity_statements;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    std::vector<architecture_body*> architectures;
};

struct architecture_body {
    std::string identifier;
    name_node* primary{nullptr};
    std::vector<block_declarative_item> block_declarative_items;
    std::vector<concurrent_statement*> concurrent_statements;
    // elaborated members
    std::vector<ast::use_clause*> packages_in_scope;
    design_file* my_file;
    entity_declaration* primary_ref{nullptr};

    architecture_body* clone(ast_node_factory&);
};

struct design_file {
    std::vector<unit_item> units;
    std::string lib_name;
};

struct sequential_statement {
    std::string label;
    sequential_statement_item stmt;
};

struct wait_statement {
    std::vector<std::string> sensitivity_list;
    expression_item condition_clause;
    expression_item timeout_clause;
    // elaborated members
    std::vector<declaration_ref> sensitivity_refs;
};

struct assertion_statement {
    expression_item condition;
    expression_item report;
    expression_item severity;
};

struct report_statement {
    expression_item report;
    expression_item severity;
};

struct simple_waveform_assignment {
    target_item target;
    delay_mechanism_e delay_mechanism_type{delay_mechanism_e::TRANSPORT};
    expression_item delay_mechanism_reject;
    std::vector<waveform_element> waveform;
};

struct simple_force_assignment {
    target_item target;
    force_mode_e force_mode;
    std::vector<waveform_element> waveform;
};

struct simple_release_assignment {
    target_item target;
    force_mode_e force_mode;
};

struct conditional_waveform_assignment {
    target_item target;
    delay_mechanism_e delay_mechanism_type{delay_mechanism_e::TRANSPORT};
    expression_item delay_mechanism_reject;
    std::vector<conditional_waveform_element> conditional_waveforms;
};

struct conditional_expression_element {
    expression_item value;
    expression_item condition;
};

struct conditional_force_assignment {
    target_item target;
    force_mode_e force_mode;
    std::vector<conditional_expression_element*> conditional_expressions;
};

struct selected_waveform_assignment {
    expression_item with_expression;
    bool is_questionable{false};
    target_item target;
    delay_mechanism_e delay_mechanism_type{delay_mechanism_e::TRANSPORT};
    expression_item delay_mechanism_reject;
    std::vector<selected_waveform_element*> selected_waveforms;
};

struct selected_expression_element {
    waveform_element waveform;
    std::vector<choice_item> choices;
};

struct selected_force_assignment {
    expression_item with_expression;
    bool is_questionable{false};
    target_item target;
    force_mode_e force_mode;
    std::vector<selected_expression_element*> selected_expressions;
};

struct simple_variable_assignment {
    target_item target;
    expression_item value;
};
struct conditional_variable_assignment {
    target_item target;
    std::vector<conditional_expression_element*> conditional_expressions;
};
struct selected_variable_assignment {
    expression_item with_expression;
    bool is_questionable{false};
    target_item target;
    std::vector<selected_expression_element*> selected_expressions;
};

struct procedure_call_statement {
    std::string name;
    // elaborated members
    declaration_ref procedure_ref;
};

struct if_statement {
    std::vector<std::tuple<expression_item, std::vector<sequential_statement_item>>> if_clauses;
    std::vector<sequential_statement_item> else_clauses;
};

struct case_statement_alternative {
    std::vector<choice_item> choices;
    std::vector<sequential_statement_item> statements;
};

struct case_statement {
    expression_item expression;
    std::vector<case_statement_alternative> alternatives;
};

struct parameter_specification {
    std::string for_identifier;
    discrete_range_item range;
};

struct loop_statement {
    loop_expression_item iteration_scheme;
    std::vector<sequential_statement_item> statements;
};

struct next_statement {
    std::string label;
    expression_item exit_expression;
};

struct exit_statement {
    std::string label;
    expression_item exit_expression;
};

struct return_statement {
    expression_item return_expression;
};

struct null_statement {};

} // namespace ast
