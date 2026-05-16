#pragma once

#include "elaborator.h"
#include <string>
#include <type_traits>

namespace vhdl_fe {
struct linker {
    elaborator& elab;

    explicit linker(elaborator& elab_)
    : elab(elab_) {}

    void run() {
        for(const auto& unit : elab.secondary_units) {
            std::visit(
                [this](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::architecture_body>) {
                        link_concurrent_statements(file_lib(node->my_file), node->concurrent_statements);
                    }
                },
                unit);
        }
    }

private:
    static std::string file_lib(const ast::design_file* file) { return file && !file->lib_name.empty() ? file->lib_name : "work"; }

    void bind_from_configuration(ast::component_instantiation_statement* inst, ast::configuration_declaration* config) {
        if(!inst || !config)
            return;

        if(config->entity_ref)
            inst->entity_ref = config->entity_ref;

        std::string architecture_name;
        if(config->block_config && config->block_config->block_spec)
            architecture_name = config->block_config->block_spec->label;

        inst->architecture_ref = elab.find_architecture(file_lib(config->my_file), config->name, architecture_name);
        if(!inst->entity_ref && inst->architecture_ref)
            inst->entity_ref = inst->architecture_ref->primary_ref;
    }

    void bind_by_component_name(const std::string& lib, ast::component_instantiation_statement* inst) {
        if(!inst || !inst->component_ref)
            return;

        inst->entity_ref = elab.find_entity(lib, inst->component_ref->identifier);
        if(inst->entity_ref)
            inst->architecture_ref = elab.find_architecture(file_lib(inst->entity_ref->my_file), inst->entity_ref->identifier, {});
    }

    void link_component_instantiation(const std::string& lib, ast::component_instantiation_statement* inst) {
        if(!inst)
            return;

        if(inst->unit_kind == ast::instantiated_unit_kind_e::CONFIGURATION && !inst->configuration_ref)
            inst->configuration_ref = elab.find_configuration(lib, inst->unit_name);

        if(inst->configuration_ref)
            bind_from_configuration(inst, inst->configuration_ref);
        else if(inst->unit_kind == ast::instantiated_unit_kind_e::COMPONENT && !inst->entity_ref)
            bind_by_component_name(lib, inst);


        if(inst->entity_ref)
            elab.instantiated_entities.insert(inst->entity_ref);
    }

    void link_generate_body(const std::string& lib, const ast::generate_statement_body& body) {
        link_concurrent_statements(lib, body.concurrent_statements);
    }

    void link_concurrent_statement(const std::string& lib, ast::concurrent_statement* stmt) {
        if(!stmt)
            return;

        std::visit(
            [this, &lib](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::component_instantiation_statement>) {
                    link_component_instantiation(lib, node);
                } else if constexpr(std::is_same_v<T, ast::block_statement>) {
                    link_concurrent_statements(lib, node->concurrent_statements);
                } else if constexpr(std::is_same_v<T, ast::for_generate_statement>) {
                    link_generate_body(lib, node->body);
                } else if constexpr(std::is_same_v<T, ast::if_generate_statement>) {
                    for(auto* clause : node->clauses) {
                        if(clause)
                            link_generate_body(lib, clause->body);
                    }
                    link_generate_body(lib, node->else_body);
                } else if constexpr(std::is_same_v<T, ast::case_generate_statement>) {
                    for(auto* alt : node->alternatives) {
                        if(alt)
                            link_generate_body(lib, alt->body);
                    }
                }
            },
            stmt->statement);
    }

    void link_concurrent_statements(const std::string& lib, const std::vector<ast::concurrent_statement*>& statements) {
        for(auto* stmt : statements)
            link_concurrent_statement(lib, stmt);
    }
};
} // namespace vhdl_fe
