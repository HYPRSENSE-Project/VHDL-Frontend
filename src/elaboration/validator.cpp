// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#include "validator.h"
#include <cctype>
#include <string_view>
#include <syntax_error_logger.h>
#include <type_traits>

namespace vhdl_fe {
namespace {

bool empty_ref(const ast::declaration_ref& ref) { return std::holds_alternative<std::monostate>(ref); }

bool is_all_name(std::string_view value) {
    return value.size() == 3 && std::tolower(static_cast<unsigned char>(value[0])) == 'a' &&
           std::tolower(static_cast<unsigned char>(value[1])) == 'l' && std::tolower(static_cast<unsigned char>(value[2])) == 'l';
}

bool is_others_name(std::string_view value) {
    return value.size() == 6 && std::tolower(static_cast<unsigned char>(value[0])) == 'o' &&
           std::tolower(static_cast<unsigned char>(value[1])) == 't' && std::tolower(static_cast<unsigned char>(value[2])) == 'h' &&
           std::tolower(static_cast<unsigned char>(value[3])) == 'e' && std::tolower(static_cast<unsigned char>(value[4])) == 'r' &&
           std::tolower(static_cast<unsigned char>(value[5])) == 's';
}

bool looks_like_literal(std::string_view text) {
    if(text.empty())
        return true;
    if(text == "null" || text == "NULL")
        return true;
    if(std::isdigit(static_cast<unsigned char>(text.front())))
        return true;
    return text.find('"') != std::string_view::npos || text.find('\'') != std::string_view::npos;
}

std::string name_text(const ast::name_node* node) { return node ? node->value : std::string(); }

std::string type_mark_text(const ast::type_mark* node) { return node && node->name ? node->name->value : std::string(); }

std::string full_selected_name(const ast::selected_name* selected) {
    if(selected->suffix.empty())
        return selected->identifier;
    return selected->identifier + "." + selected->suffix;
}

std::string full_selected_name(const ast::used_package* used) {
    if(!used)
        return {};
    std::string out = used->identifier;
    for(const auto& suffix : used->suffixes) {
        if(!out.empty())
            out += ".";
        out += suffix;
    }
    return out;
}

std::string package_name(const ast::used_package* used) {
    if(!used)
        return {};
    std::string out = used->identifier;
    const auto count = used->suffixes.empty() ? 0U : used->suffixes.size() - 1U;
    for(size_t i = 0; i < count; ++i) {
        if(!out.empty())
            out += ".";
        out += used->suffixes[i];
    }
    return out;
}

class resolved_ast_validator {
public:
    std::vector<ast::error_data> run(const std::vector<ast::design_file*>& design_files) {
        for(auto* file : design_files)
            validate_design_file(file);
        return diagnostics;
    }

private:
    std::vector<ast::error_data> diagnostics;

    void add_unresolved(std::string what, const std::string& name, ast::source_loc* src = nullptr) {
        ast::source_loc loc;
        if(src)
            loc = *src;
        auto message = std::move(what);
        if(!name.empty()) {
            message += ": ";
            message += name;
        }
        diagnostics.push_back({ast::error_data::error_kind_t::VALIDATIONERROR, std::move(message), loc.file, loc.line, loc.col});
    }

    void require_ref(const ast::declaration_ref& ref, std::string what, const std::string& name, ast::source_loc* src = nullptr) {
        if(empty_ref(ref))
            add_unresolved(std::move(what), name, src);
    }

    template <typename T> void require_ptr(T* ptr, std::string what, const std::string& name, ast::source_loc* src = nullptr) {
        if(!ptr)
            add_unresolved(std::move(what), name, src);
    }

    void validate_design_file(ast::design_file* file) {
        if(!file)
            return;
        for(const auto& unit : file->units)
            validate_unit(unit);
    }

    void validate_unit(const ast::unit_item& unit) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::library_clause>) {
                    return;
                } else if constexpr(std::is_same_v<T, ast::use_clause>) {
                    validate_use_clause(node);
                } else if constexpr(std::is_same_v<T, ast::context_reference>) {
                    validate_context_reference(node);
                } else if constexpr(std::is_same_v<T, ast::entity_declaration>) {
                    validate_entity(node);
                } else if constexpr(std::is_same_v<T, ast::configuration_declaration>) {
                    validate_configuration(node);
                } else if constexpr(std::is_same_v<T, ast::package_declaration>) {
                    validate_package(node);
                } else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>) {
                    validate_package_instantiation(node);
                } else if constexpr(std::is_same_v<T, ast::context_declaration>) {
                    validate_context(node);
                } else if constexpr(std::is_same_v<T, ast::architecture_body>) {
                    validate_architecture(node);
                } else if constexpr(std::is_same_v<T, ast::package_body>) {
                    validate_package_body(node);
                }
            },
            unit);
    }

    void validate_use_clause(ast::use_clause* use_clause) {
        if(!use_clause)
            return;
        for(auto* clause : use_clause->clauses)
            validate_used_package(clause);
    }

    void validate_used_package(ast::used_package* clause) {
        if(!clause || clause->suffixes.empty())
            return;
        const auto name = full_selected_name(clause);
        const auto package = package_name(clause);
        const auto& suffix = clause->suffixes.back();
        if(is_all_name(suffix) || is_others_name(suffix)) {
            require_ptr(clause->package_ref, "unresolved package reference", package);
            return;
        }
        if(clause->suffixes.size() > 1)
            require_ptr(clause->package_ref, "unresolved package reference", package);
        require_ref(clause->selected_ref, "unresolved selected use reference", name, clause);
    }

    void validate_context_reference(ast::context_reference* ref) {
        if(!ref)
            return;
        for(const auto& selected : ref->selected_names)
            require_ref(selected->resolved_ref, "unresolved context reference", full_selected_name(selected), selected);
    }

    template <typename Vector> void validate_interfaces(const Vector& interfaces) {
        for(const auto& item : interfaces)
            validate_interface(item);
    }

    void validate_interface(const ast::interface_declaration_item& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::interface_constant_declaration> ||
                             std::is_same_v<T, ast::interface_signal_declaration> ||
                             std::is_same_v<T, ast::interface_variable_declaration> || std::is_same_v<T, ast::interface_file_declaration>) {
                    validate_subtype_indication(node->subtype_indic);
                    if constexpr(!std::is_same_v<T, ast::interface_file_declaration>)
                        validate_expression(node->expression);
                } else if constexpr(std::is_same_v<T, ast::interface_subprogram_declaration>) {
                    std::visit(
                        [this](auto* spec) {
                            if(!spec)
                                return;
                            using S = std::decay_t<decltype(*spec)>;
                            validate_interfaces(spec->formal_parameter_list);
                            if constexpr(std::is_same_v<S, ast::interface_function_specification>) {
                                require_ref(spec->return_type_ref, "unresolved interface function return type",
                                            type_mark_text(spec->return_type_mark), spec->return_type_mark);
                            }
                        },
                        node->nterface_subprogram_specification);
                } else if constexpr(std::is_same_v<T, ast::interface_package_declaration>) {
                    require_ptr(node->package_ref, "unresolved interface package reference", name_text(node->name));
                    validate_associations(node->generic_map_aspect, false);
                }
            },
            item);
    }

    template <typename Vector> void validate_declarations(const Vector& declarations) {
        for(const auto& item : declarations)
            validate_declaration(item);
    }

    template <typename Variant> void validate_declaration(const Variant& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::signal_declaration>) {
                    require_ref(node->type_ref, "unresolved signal type", type_mark_text(node->type));
                    validate_resolution(node->resolution);
                    validate_constraint(node->constraint);
                } else if constexpr(std::is_same_v<T, ast::component_declaration>) {
                    validate_interfaces(node->generic_list);
                    validate_interfaces(node->port_list);
                } else if constexpr(std::is_same_v<T, ast::configuration_specification>) {
                    validate_component_specification(node->component_spec);
                    validate_binding(node->binding);
                } else if constexpr(std::is_same_v<T, ast::type_declaration>) {
                    validate_type_definition(node->type);
                } else if constexpr(std::is_same_v<T, ast::subtype_declaration>) {
                    validate_subtype_indication(node->indication);
                } else if constexpr(std::is_same_v<T, ast::constant_declaration>) {
                    validate_subtype_indication(node->indication);
                    validate_expression(node->expr);
                } else if constexpr(std::is_same_v<T, ast::variable_declaration>) {
                    validate_subtype_indication(node->indication);
                    validate_expression(node->expr);
                } else if constexpr(std::is_same_v<T, ast::file_declaration>) {
                    validate_subtype_indication(node->indication);
                    validate_expression(node->open_expression);
                    validate_expression(node->file_logical_name);
                } else if constexpr(std::is_same_v<T, ast::alias_declaration>) {
                    validate_subtype_indication(node->indication);
                    if(!node->name.empty())
                        require_ref(node->name_ref, "unresolved alias target", node->name, node);
                    if(const auto* signature = node->signatue) {
                        for(size_t i = 0; i < signature->type_marks.size(); ++i) {
                            const auto name = type_mark_text(signature->type_marks[i]);
                            if(i >= node->type_mark_refs.size())
                                add_unresolved("unresolved alias signature type", name);
                            else
                                require_ref(node->type_mark_refs[i], "unresolved alias signature type", name, signature->type_marks[i]);
                        }
                        if(signature->return_type_mark)
                            require_ref(node->return_type_mark_ref, "unresolved alias return type",
                                        type_mark_text(signature->return_type_mark), signature->return_type_mark);
                    }
                } else if constexpr(std::is_same_v<T, ast::attribute_declaration>) {
                    require_ref(node->type_ref, "unresolved attribute type", type_mark_text(node->type), node->type);
                } else if constexpr(std::is_same_v<T, ast::attribute_specification>) {
                    if(std::holds_alternative<ast::entity_designator_list*>(node->entity_names)) {
                        const auto* names = std::get<ast::entity_designator_list*>(node->entity_names);
                        for(size_t i = 0; i < names->name_list.size(); ++i) {
                            const auto* designator = names->name_list[i];
                            const auto name = name_text(designator->entity_tag);
                            if(i >= node->entity_refs.size())
                                add_unresolved("unresolved attribute entity", name);
                            else
                                require_ref(node->entity_refs[i], "unresolved attribute entity", name);
                        }
                    } else {
                        const auto* name = std::get<ast::literal_node*>(node->entity_names);
                        if(node->entity_refs.empty())
                            add_unresolved("unresolved attribute entity", name ? name->text : std::string());
                        else
                            require_ref(node->entity_refs.front(), "unresolved attribute entity", name ? name->text : std::string());
                    }
                    validate_expression(node->expr);
                } else if constexpr(std::is_same_v<T, ast::group_declaration>) {
                    require_ref(node->template_ref, "unresolved group template", node->name);
                    for(size_t i = 0; i < node->group_constituent_list.size(); ++i) {
                        if(i >= node->constituent_refs.size())
                            add_unresolved("unresolved group constituent", node->group_constituent_list[i]);
                        else
                            require_ref(node->constituent_refs[i], "unresolved group constituent", node->group_constituent_list[i]);
                    }
                } else if constexpr(std::is_same_v<T, ast::disconnection_specification>) {
                    require_ref(node->signal_ref, "unresolved disconnection signal", node->signal_name);
                    require_ref(node->type_ref, "unresolved disconnection type", type_mark_text(node->type));
                    validate_expression(node->after_expression);
                } else if constexpr(std::is_same_v<T, ast::subprogram_declaration>) {
                    validate_subprogram_declaration(node);
                } else if constexpr(std::is_same_v<T, ast::subprogram_instantiation_declaration>) {
                    require_ptr(node->target_ref, "unresolved subprogram instantiation target", name_text(node->target_name));
                } else if constexpr(std::is_same_v<T, ast::subprogram_body>) {
                    if(node->specification)
                        validate_subprogram_declaration(node->specification);
                    validate_declarations(node->declarative_items);
                    for(auto* stmt : node->sequential_statements)
                        validate_sequential_statement(stmt);
                } else if constexpr(std::is_same_v<T, ast::package_declaration>) {
                    validate_package(node);
                } else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>) {
                    validate_package_instantiation(node);
                } else if constexpr(std::is_same_v<T, ast::package_body>) {
                    validate_package_body(node);
                } else if constexpr(std::is_same_v<T, ast::use_clause>) {
                    validate_use_clause(node);
                } else if constexpr(std::is_same_v<T, ast::protected_type_declaration>) {
                    validate_declarations(node->protected_type_declarative_items);
                }
            },
            item);
    }

    void validate_subprogram_declaration(ast::subprogram_declaration* subprogram) {
        if(!subprogram)
            return;
        validate_interfaces(subprogram->generic_list);
        validate_associations(subprogram->generic_map, false);
        validate_interfaces(subprogram->parameter_list);
        if(subprogram->return_type)
            require_ref(subprogram->return_type_ref, "unresolved subprogram return type", type_mark_text(subprogram->return_type));
    }

    void validate_type_definition(const ast::type_definition_item& type) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::numeric_type_definition>) {
                    std::visit([this](auto* range) { validate_constraint_node(range); }, node->range);
                } else if constexpr(std::is_same_v<T, ast::unbounded_array_definition>) {
                    for(auto* mark : node->index_subtype_definitions)
                        validate_type_mark(mark);
                } else if constexpr(std::is_same_v<T, ast::constrained_array_definition>) {
                    for(const auto& constraint : node->index_constraints)
                        validate_discrete_range(constraint);
                    validate_subtype_indication(node->subtype_indication_);
                } else if constexpr(std::is_same_v<T, ast::record_type_definition>) {
                    for(auto* elem : node->element_declarations)
                        if(elem)
                            validate_subtype_indication(elem->element_subtype_definition);
                } else if constexpr(std::is_same_v<T, ast::subtype_indication>) {
                    validate_subtype_indication(node);
                } else if constexpr(std::is_same_v<T, ast::type_mark>) {
                    validate_type_mark(node);
                } else if constexpr(std::is_same_v<T, ast::literal_node>) {
                    if(!looks_like_literal(node->text))
                        require_ref(node->resolved_ref, "unresolved type literal", node->text);
                } else if constexpr(std::is_same_v<T, ast::protected_type_definition>) {
                    validate_declarations(node->declarations);
                    validate_declarations(node->body_declarations);
                } else if constexpr(std::is_same_v<T, ast::protected_type_declaration>) {
                    validate_declarations(node->protected_type_declarative_items);
                }
            },
            type);
    }

    void validate_type_mark(ast::type_mark* mark) {
        if(mark && mark->name && !mark->name->value.empty())
            require_ref(mark->resolved_ref, "unresolved type mark", mark->name->value);
    }

    void validate_subtype_indication(ast::subtype_indication* subtype) {
        if(!subtype)
            return;
        validate_resolution(subtype->resolution);
        validate_type_mark(subtype->type);
        validate_constraint(subtype->constr);
    }

    void validate_resolution(ast::resolution_indication* resolution) {
        if(!resolution)
            return;
        if(!resolution->name.empty())
            require_ref(resolution->resolution_ref, "unresolved resolution indication", resolution->name);
        std::visit(
            [this](auto* item) {
                if constexpr(std::is_same_v<std::decay_t<decltype(item)>, ast::resolution_indication*>)
                    validate_resolution(item);
            },
            resolution->elem_resolution);
    }

    void validate_constraint(const ast::constraint_item& constraint) {
        std::visit([this](auto* node) { validate_constraint_node(node); }, constraint);
    }

    void validate_constraint_node(ast::explicit_range* range) { validate_range(range); }

    void validate_constraint_node(ast::attribute_range* range) {
        if(range && !range->name.empty())
            require_ref(range->prefix_ref, "unresolved attribute range prefix", range->name);
    }

    void validate_constraint_node(ast::array_constraint* constraint) {
        if(!constraint)
            return;
        for(const auto& item : constraint->index_constraint)
            validate_discrete_range(item);
    }

    void validate_constraint_node(ast::record_constraint* constraint) {
        if(!constraint)
            return;
        for(const auto& elem : constraint->constraint)
            validate_subtype_indication(elem.indication);
    }

    void validate_discrete_range(const ast::discrete_range_item& item) {
        std::visit(
            [this](auto* node) {
                if constexpr(std::is_same_v<std::decay_t<decltype(node)>, ast::subtype_indication*>)
                    validate_subtype_indication(node);
                else
                    validate_constraint_node(node);
            },
            item);
    }

    void validate_range(ast::explicit_range* range) {
        if(!range)
            return;
        validate_expression(range->left);
        validate_expression(range->right);
    }

    void validate_expression(const ast::expression_item& expression) {
        std::visit(
            [this](auto* node) {
                if(node)
                    validate_expression_node(node);
            },
            expression);
    }

    void validate_primary(const ast::primary_item& primary) {
        std::visit(
            [this](auto* node) {
                if(node)
                    validate_expression_node(node);
            },
            primary);
    }

    void validate_expression_node(ast::literal_node* node) {
        if(node && !looks_like_literal(node->text))
            require_ref(node->resolved_ref, "unresolved literal reference", node->text);
    }

    void validate_expression_node(ast::name_node* node) {
        if(!node || node->value.empty())
            return;
        if(node->value.front() != '"' && node->value.front() != '\'')
            require_ref(node->resolved_ref, "unresolved name", node->value);
        if(node->slice)
            validate_range(node->slice->range);
        if(node->arguments)
            validate_associations(node->arguments->associations, false);
    }

    void validate_expression_node(ast::allocator* node) {
        if(!node)
            return;
        validate_subtype_indication(node->si);
        validate_expression_node(node->qe);
    }

    void validate_expression_node(ast::aggregate* node) {
        if(!node)
            return;
        for(auto* assoc : node->elem_assoc) {
            if(!assoc)
                continue;
            for(auto* choice : assoc->choices)
                validate_expression_node(choice);
            validate_expression(assoc->expr);
        }
    }

    void validate_expression_node(ast::qualified_expression* node) {
        if(!node)
            return;
        require_ref(node->type_ref, "unresolved qualified expression type", type_mark_text(node->type));
        validate_expression_node(node->aggr);
    }

    void validate_expression_node(ast::simple_expression* node) {
        if(!node)
            return;
        validate_primary(node->first_primary);
        validate_primary(node->secondary_primary);
        validate_expression_node(node->lhs);
        validate_expression_node(node->rhs);
        validate_expression_node(node->operand);
    }

    void validate_expression_node(ast::conditional_primary* node) {
        if(node)
            validate_primary(node->prim);
    }

    void validate_expression_node(ast::binary_expression* node) {
        if(!node)
            return;
        validate_expression(node->lhs);
        validate_expression(node->rhs);
    }

    void validate_target(const ast::target_item& target) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::aggregate>)
                    validate_expression_node(node);
                else if constexpr(std::is_same_v<T, ast::name_node>)
                    validate_expression_node(node);
            },
            target);
    }

    void validate_associations(const std::vector<ast::association_element*>& associations, bool expect_formal_refs) {
        for(auto* assoc : associations)
            validate_association(assoc, expect_formal_refs);
    }

    void validate_association(ast::association_element* assoc, bool expect_formal_refs) {
        if(!assoc)
            return;
        if(expect_formal_refs && !assoc->formal_name.empty())
            require_ref(assoc->formal_ref, "unresolved formal association", assoc->formal_name);
        if(!assoc->actual_name.empty())
            require_ref(assoc->actual_ref, "unresolved actual association", assoc->actual_name);
        validate_actual_designator(assoc->act_designator);
    }

    void validate_actual_designator(ast::actual_designator* designator) {
        if(!designator)
            return;
        validate_expression(designator->expr);
        validate_subtype_indication(designator->indication);
    }

    void validate_component_specification(ast::component_specification* spec) {
        if(spec && !spec->name.empty())
            require_ptr(spec->component_ref, "unresolved component specification", spec->name);
    }

    void validate_binding(ast::binding_indication* binding) {
        if(!binding || binding->type == ast::entity_aspect_e::OPEN)
            return;

        if(binding->type == ast::entity_aspect_e::ENTITY) {
            require_ptr(binding->entity_ref, "unresolved binding entity", binding->unit_ref);
            require_ptr(binding->architecture_ref, "unresolved binding architecture",
                        binding->identifier.empty() ? binding->unit_ref : binding->unit_ref + "." + binding->identifier);
            validate_associations(binding->generic_map, binding->entity_ref != nullptr);
            validate_associations(binding->port_map, binding->entity_ref != nullptr);
        } else {
            require_ptr(binding->configuration_ref, "unresolved binding configuration", binding->unit_ref);
            validate_associations(binding->generic_map, false);
            validate_associations(binding->port_map, false);
        }
    }

    void validate_generate_specification(const ast::generate_specification_item& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::subtype_indication>)
                    validate_subtype_indication(node);
                else if constexpr(std::is_same_v<T, ast::attribute_range> || std::is_same_v<T, ast::explicit_range>)
                    validate_constraint_node(node);
                else
                    validate_expression_node(node);
            },
            item);
    }

    void validate_block_configuration(ast::block_configuration* block) {
        if(!block)
            return;
        if(block->block_spec)
            validate_generate_specification(block->block_spec->generate_specification);
        for(auto* use : block->use_clauses)
            validate_use_clause(use);
        for(const auto& item : block->configuration_items)
            validate_configuration_item(item);
    }

    void validate_configuration_item(const ast::configuration_item& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::block_configuration>)
                    validate_block_configuration(node);
                else if constexpr(std::is_same_v<T, ast::component_configuration>)
                    validate_component_configuration(node);
            },
            item);
    }

    void validate_component_configuration(ast::component_configuration* node) {
        if(!node)
            return;
        validate_component_specification(node->component_spec);
        validate_binding(node->binding);
        validate_block_configuration(node->block_config);
    }

    void validate_entity_statement(ast::entity_statement* stmt) {
        if(!stmt)
            return;
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::process_statement>)
                    validate_process(node);
                else if constexpr(std::is_same_v<T, ast::concurrent_procedure_call_statement>)
                    require_ref(node->procedure_ref, "unresolved concurrent procedure", node->name);
                else if constexpr(std::is_same_v<T, ast::concurrent_assertion_statement>) {
                    validate_expression(node->condition);
                    validate_expression(node->report);
                    validate_expression(node->severity);
                }
            },
            stmt->statement);
    }

    void validate_concurrent_statement(ast::concurrent_statement* stmt) {
        if(!stmt)
            return;
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::component_instantiation_statement>) {
                    if(node->unit_kind == ast::instantiated_unit_kind_e::ENTITY) {
                        require_ptr(node->entity_ref, "unresolved instantiated entity", node->unit_name);
                        require_ptr(node->architecture_ref, "unresolved instantiated architecture",
                                    node->architecture_id.empty() ? node->unit_name : node->unit_name + "." + node->architecture_id);
                    } else if(node->unit_kind == ast::instantiated_unit_kind_e::CONFIGURATION) {
                        require_ptr(node->configuration_ref, "unresolved instantiated configuration", node->unit_name);
                        if(node->configuration_ref)
                            require_ptr(node->entity_ref, "unresolved configuration entity", node->unit_name);
                    } else {
                        require_ptr(node->component_ref, "unresolved component instantiation", node->unit_name);
                        require_ptr(node->entity_ref, "unresolved component binding entity", node->unit_name);
                        require_ptr(node->architecture_ref, "unresolved component binding architecture", node->unit_name);
                    }
                    const auto has_formal_scope = node->component_ref != nullptr || node->entity_ref != nullptr;
                    validate_associations(node->generic_map, has_formal_scope);
                    validate_associations(node->port_map, has_formal_scope);
                } else if constexpr(std::is_same_v<T, ast::block_statement>) {
                    validate_block_statement(node);
                } else if constexpr(std::is_same_v<T, ast::for_generate_statement>) {
                    validate_generate_body(node->body);
                } else if constexpr(std::is_same_v<T, ast::if_generate_statement>) {
                    for(auto* clause : node->clauses) {
                        if(!clause)
                            continue;
                        validate_expression(clause->condition);
                        validate_generate_body(clause->body);
                    }
                    validate_generate_body(node->else_body);
                } else if constexpr(std::is_same_v<T, ast::case_generate_statement>) {
                    for(auto* alt : node->alternatives)
                        if(alt)
                            validate_generate_body(alt->body);
                } else if constexpr(std::is_same_v<T, ast::process_statement>) {
                    validate_process(node);
                } else if constexpr(std::is_same_v<T, ast::concurrent_procedure_call_statement>) {
                    require_ref(node->procedure_ref, "unresolved concurrent procedure", node->name);
                } else if constexpr(std::is_same_v<T, ast::concurrent_assertion_statement>) {
                    validate_expression(node->condition);
                    validate_expression(node->report);
                    validate_expression(node->severity);
                } else if constexpr(std::is_same_v<T, ast::concurrent_signal_assignment_any>) {
                    validate_target(node->target);
                    validate_expression(node->delay_mechanism_reject);
                    for(auto* waveform : node->waveform)
                        validate_waveform(waveform);
                    for(auto* conditional : node->conditional_waveforms) {
                        if(!conditional)
                            continue;
                        for(auto* waveform : conditional->waveform)
                            validate_waveform(waveform);
                        validate_expression(conditional->condition);
                    }
                } else if constexpr(std::is_same_v<T, ast::concurrent_selected_signal_assignment>) {
                    validate_expression(node->with_expression);
                    validate_target(node->target);
                    validate_expression(node->delay_mechanism_reject);
                    for(auto* selected : node->selected_waveforms)
                        validate_selected_waveform(selected);
                }
            },
            stmt->statement);
    }

    void validate_block_statement(ast::block_statement* block) {
        if(!block)
            return;
        validate_expression(block->condition);
        validate_interfaces(block->generic_list);
        validate_interfaces(block->port_list);
        validate_associations(block->generic_map, true);
        validate_associations(block->port_map, true);
        validate_declarations(block->block_declarative_items);
        for(auto* stmt : block->concurrent_statements)
            validate_concurrent_statement(stmt);
    }

    void validate_generate_body(ast::generate_statement_body& body) {
        validate_declarations(body.block_declarative_items);
        for(auto* stmt : body.concurrent_statements)
            validate_concurrent_statement(stmt);
    }

    void validate_process(ast::process_statement* process) {
        if(!process)
            return;
        for(size_t i = 0; i < process->sensitivity_list.size(); ++i) {
            if(i >= process->sensitivity_refs.size())
                add_unresolved("unresolved process sensitivity", process->sensitivity_list[i]);
            else
                require_ref(process->sensitivity_refs[i], "unresolved process sensitivity", process->sensitivity_list[i]);
        }
        validate_declarations(process->declarative_items);
        for(auto* stmt : process->sequential_statements)
            validate_sequential_statement(stmt);
    }

    void validate_waveform(ast::waveform_element* waveform) {
        if(!waveform)
            return;
        validate_expression(waveform->value);
        validate_expression(waveform->after_expression);
    }

    void validate_selected_waveform(ast::selected_waveform_element* selected) {
        if(!selected)
            return;
        for(auto* waveform : selected->waveform)
            validate_waveform(waveform);
        for(const auto& choice : selected->choices)
            validate_choice(choice);
    }

    void validate_choice(const ast::choice_item& choice) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::subtype_indication>)
                    validate_subtype_indication(node);
                else if constexpr(std::is_same_v<T, ast::attribute_range> || std::is_same_v<T, ast::explicit_range>)
                    validate_constraint_node(node);
                else
                    validate_expression_node(node);
            },
            choice);
    }

    void validate_sequential_statement(ast::sequential_statement* stmt) {
        if(stmt)
            validate_sequential_item(stmt->stmt);
    }

    void validate_sequential_item(const ast::sequential_statement_item& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::wait_statement>) {
                    for(size_t i = 0; i < node->sensitivity_list.size(); ++i) {
                        if(i >= node->sensitivity_refs.size())
                            add_unresolved("unresolved wait sensitivity", node->sensitivity_list[i]);
                        else
                            require_ref(node->sensitivity_refs[i], "unresolved wait sensitivity", node->sensitivity_list[i]);
                    }
                    validate_expression(node->condition_clause);
                    validate_expression(node->timeout_clause);
                } else if constexpr(std::is_same_v<T, ast::assertion_statement>) {
                    validate_expression(node->condition);
                    validate_expression(node->report);
                    validate_expression(node->severity);
                } else if constexpr(std::is_same_v<T, ast::report_statement>) {
                    validate_expression(node->report);
                    validate_expression(node->severity);
                } else if constexpr(std::is_same_v<T, ast::simple_waveform_assignment>) {
                    validate_target(node->target);
                    validate_expression(node->delay_mechanism_reject);
                    for(auto& waveform : node->waveform)
                        validate_waveform_value(waveform);
                } else if constexpr(std::is_same_v<T, ast::simple_force_assignment>) {
                    validate_target(node->target);
                    for(auto& waveform : node->waveform)
                        validate_waveform_value(waveform);
                } else if constexpr(std::is_same_v<T, ast::simple_release_assignment>) {
                    validate_target(node->target);
                } else if constexpr(std::is_same_v<T, ast::conditional_waveform_assignment>) {
                    validate_target(node->target);
                    validate_expression(node->delay_mechanism_reject);
                    for(auto& conditional : node->conditional_waveforms)
                        validate_conditional_waveform_value(conditional);
                } else if constexpr(std::is_same_v<T, ast::conditional_force_assignment>) {
                    validate_target(node->target);
                    for(auto* conditional : node->conditional_expressions)
                        validate_conditional_expression(conditional);
                } else if constexpr(std::is_same_v<T, ast::selected_waveform_assignment>) {
                    validate_expression(node->with_expression);
                    validate_target(node->target);
                    validate_expression(node->delay_mechanism_reject);
                    for(auto* selected : node->selected_waveforms)
                        validate_selected_waveform(selected);
                } else if constexpr(std::is_same_v<T, ast::selected_force_assignment>) {
                    validate_expression(node->with_expression);
                    validate_target(node->target);
                    for(auto* selected : node->selected_expressions)
                        validate_selected_expression(selected);
                } else if constexpr(std::is_same_v<T, ast::simple_variable_assignment>) {
                    validate_target(node->target);
                    validate_expression(node->value);
                } else if constexpr(std::is_same_v<T, ast::conditional_variable_assignment>) {
                    validate_target(node->target);
                    for(auto* conditional : node->conditional_expressions)
                        validate_conditional_expression(conditional);
                } else if constexpr(std::is_same_v<T, ast::selected_variable_assignment>) {
                    validate_expression(node->with_expression);
                    validate_target(node->target);
                    for(auto* selected : node->selected_expressions)
                        validate_selected_expression(selected);
                } else if constexpr(std::is_same_v<T, ast::procedure_call_statement>) {
                    require_ref(node->procedure_ref, "unresolved procedure call", node->name);
                } else if constexpr(std::is_same_v<T, ast::if_statement>) {
                    for(const auto& clause : node->if_clauses) {
                        validate_expression(std::get<0>(clause));
                        for(const auto& nested : std::get<1>(clause))
                            validate_sequential_item(nested);
                    }
                    for(const auto& nested : node->else_clauses)
                        validate_sequential_item(nested);
                } else if constexpr(std::is_same_v<T, ast::case_statement>) {
                    validate_expression(node->expression);
                    for(const auto& alt : node->alternatives) {
                        for(const auto& choice : alt.choices)
                            validate_choice(choice);
                        for(const auto& nested : alt.statements)
                            validate_sequential_item(nested);
                    }
                } else if constexpr(std::is_same_v<T, ast::loop_statement>) {
                    std::visit(
                        [this](auto* iteration) {
                            if(!iteration)
                                return;
                            using I = std::decay_t<decltype(*iteration)>;
                            if constexpr(std::is_same_v<I, ast::parameter_specification>)
                                validate_discrete_range(iteration->range);
                            else
                                validate_expression_node(iteration);
                        },
                        node->iteration_scheme);
                    for(const auto& nested : node->statements)
                        validate_sequential_item(nested);
                } else if constexpr(std::is_same_v<T, ast::next_statement>) {
                    validate_expression(node->exit_expression);
                } else if constexpr(std::is_same_v<T, ast::exit_statement>) {
                    validate_expression(node->exit_expression);
                } else if constexpr(std::is_same_v<T, ast::return_statement>) {
                    validate_expression(node->return_expression);
                }
            },
            item);
    }

    void validate_waveform_value(ast::waveform_element& waveform) {
        validate_expression(waveform.value);
        validate_expression(waveform.after_expression);
    }

    void validate_conditional_waveform_value(ast::conditional_waveform_element& conditional) {
        for(auto* waveform : conditional.waveform)
            validate_waveform(waveform);
        validate_expression(conditional.condition);
    }

    void validate_conditional_expression(ast::conditional_expression_element* conditional) {
        if(!conditional)
            return;
        validate_expression(conditional->value);
        validate_expression(conditional->condition);
    }

    void validate_selected_expression(ast::selected_expression_element* selected) {
        if(!selected)
            return;
        validate_waveform_value(selected->waveform);
        for(const auto& choice : selected->choices)
            validate_choice(choice);
    }

    void validate_entity(ast::entity_declaration* entity) {
        if(!entity)
            return;
        validate_interfaces(entity->generic_list);
        validate_interfaces(entity->port_list);
        validate_declarations(entity->entity_declarative_items);
        for(auto* stmt : entity->entity_statements)
            validate_entity_statement(stmt);
    }

    void validate_architecture(ast::architecture_body* arch) {
        if(!arch)
            return;
        require_ptr(arch->primary_ref, "unresolved architecture primary", name_text(arch->primary));
        validate_declarations(arch->block_declarative_items);
        for(auto* stmt : arch->concurrent_statements)
            validate_concurrent_statement(stmt);
    }

    void validate_package(ast::package_declaration* package) {
        if(!package)
            return;
        validate_interfaces(package->generic_list);
        validate_associations(package->generic_map, false);
        validate_declarations(package->declarations);
    }

    void validate_package_body(ast::package_body* body) {
        if(!body)
            return;
        require_ptr(body->package_ref, "unresolved package body target", body->identifier);
        validate_declarations(body->declarative_items);
    }

    void validate_package_instantiation(ast::package_instantiation_declaration* inst) {
        if(!inst)
            return;
        require_ptr(inst->target_package_ref, "unresolved package instantiation target", name_text(inst->target_name));
        validate_associations(inst->generic_map, inst->target_package_ref != nullptr);
    }

    void validate_configuration(ast::configuration_declaration* config) {
        if(!config)
            return;
        require_ptr(config->entity_ref, "unresolved configuration entity", config->name);
        validate_declarations(config->declarative_items);
        validate_block_configuration(config->block_config);
    }

    void validate_context(ast::context_declaration* context) {
        if(!context)
            return;
        for(const auto& item : context->context_items)
            validate_context_item(item);
    }

    void validate_context_item(const ast::context_item& item) {
        std::visit(
            [this](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::use_clause>)
                    validate_use_clause(node);
                else if constexpr(std::is_same_v<T, ast::context_reference>)
                    validate_context_reference(node);
            },
            item);
    }
};

} // namespace

std::vector<ast::error_data> validate_resolved_ast(const std::vector<ast::design_file*>& design_files) {
    return resolved_ast_validator{}.run(design_files);
}

} // namespace vhdl_fe
