// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#include "elaborator.h"
#include "ast_nodes.h"
#include "error_data.h"
#include "linker.h"
#include "resolver.h"
#include <array>
#include <cctype>
#include <tuple>
#include <type_traits>

namespace vhdl_fe {

elaborator::elaborator(parser::parser& p)
: parser(p) {}

void elaborator::add_diagnostic(ast::error_data::error_kind_t kind, std::string message) {
    diagnostics.push_back({kind, std::move(message)});
}

ast::entity_declaration* elaborator::find_entity(const std::string& lib_name, const std::string& name) {
    auto [lib, entity_name] = library_and_design_name(lib_name, name);
    if(entity_name.empty())
        return nullptr;
    auto it = entities_by_key.find(make_key(lib, entity_name));
    if(it != entities_by_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("cannot resolve entity ") + name);
    return nullptr;
}

ast::architecture_body* elaborator::find_architecture(const std::string& lib_name, const std::string& entity_name,
                                                      const std::string& architecture_name) {
    auto [lib, entity] = library_and_design_name(lib_name, entity_name);
    if(entity.empty())
        return nullptr;

    if(architecture_name.empty()) {
        auto it = architectures_by_entity_key.find(make_key(lib, entity));
        if(it != architectures_by_entity_key.end() && !it->second.empty())
            return it->second.back();
        add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR,
                       std::string("cannot resolve architecture for entity ") + entity_name);
        return nullptr;
    }

    auto it = architectures_by_key.find(make_arch_key(lib, entity, architecture_name));
    if(it != architectures_by_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR,
                   std::string("cannot resolve architecture ") + architecture_name + " of entity " + entity_name);
    return nullptr;
}

std::vector<ast::architecture_body*> elaborator::find_architectures_for_entity(const std::string& lib_name,
                                                                               const std::string& entity_name) {
    auto [lib, entity] = library_and_design_name(lib_name, entity_name);
    auto it = architectures_by_entity_key.find(make_key(lib, entity));
    if(it != architectures_by_entity_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("cannot find any architecture for entity ") + entity_name);
    return {};
}

ast::configuration_declaration* elaborator::find_configuration(const std::string& lib_name, const std::string& name) {
    auto [lib, config_name] = library_and_design_name(lib_name, name);
    if(config_name.empty())
        return nullptr;
    auto it = configurations_by_key.find(make_key(lib, config_name));
    if(it != configurations_by_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("cannot resolve configuration ") + config_name);
    return nullptr;
}

ast::package_declaration* elaborator::find_package(const std::string& lib_name, const std::string& name) {
    auto [lib, package_name] = library_and_design_name(lib_name, name);
    if(package_name.empty())
        return nullptr;
    auto it = packages_by_key.find(make_key(lib, package_name));
    if(it != packages_by_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("cannot resolve package ") + name);
    return nullptr;
}

ast::context_declaration* elaborator::find_context(const std::string& lib_name, const std::string& name) {
    auto [lib, context_name] = library_and_design_name(lib_name, name);
    if(context_name.empty())
        return nullptr;
    auto it = contexts_by_key.find(make_key(lib, context_name));
    if(it != contexts_by_key.end())
        return it->second;
    add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("cannot resolve context ") + name);
    return nullptr;
}

const std::vector<ast::error_data>& elaborator::get_diagnostics() const { return diagnostics; }

void elaborator::add_design_files(std::vector<ast::design_file*> const& file_set) {
    std::unique_lock<std::mutex> lock(files_mtx);
    auto register_design_unit = [this](auto* node, const std::string& key, const char* kind, auto& map) {
        auto [it, inserted] = map.emplace(key, node);
        if(!inserted && it->second != node) {
            add_diagnostic(ast::error_data::error_kind_t::ELABORATIONERROR, std::string("duplicate ") + kind + " declaration: " + key);
        }
    };
    for(auto* df : file_set) {
        if(!df)
            continue;
        std::vector<ast::library_clause*> lib_clauses;
        std::vector<ast::use_clause*> use_clauses;
        files_by_lib[file_lib(df)].push_back(df);
        for(auto unit : df->units) {
            std::visit(
                [this, &register_design_unit, &use_clauses, &lib_clauses, df](auto* node) {
                    if(!node)
                        return;
                    const auto lib = file_lib(df);
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::library_clause>) {
                        lib_clauses.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::use_clause>) {
                        use_clauses.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::context_reference>) {
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::entity_declaration>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "entity", this->entities_by_key);
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::configuration_declaration>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "configuration", this->configurations_by_key);
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::package_declaration>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "package", this->packages_by_key);
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "package instantiation",
                                             this->package_instances_by_key);
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::context_declaration>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "context", this->contexts_by_key);
                        this->primary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::architecture_body>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        auto [primary_lib, entity_name] =
                            library_and_design_name(lib, node->primary ? node->primary->value : ""); // TODO: fix name resolution
                        const auto entity_key = make_key(primary_lib, entity_name);
                        this->architectures_by_entity_key[entity_key].push_back(node);
                        register_design_unit(node, make_arch_key(primary_lib, entity_name, node->identifier), "architecture",
                                             this->architectures_by_key);
                        this->secondary_units.push_back(node);
                    } else if constexpr(std::is_same_v<T, ast::package_body>) {
                        node->packages_in_scope.insert(node->packages_in_scope.end(), use_clauses.begin(), use_clauses.end());
                        node->my_file = df;
                        register_design_unit(node, make_key(lib, node->identifier), "package body", this->package_bodies_by_key);
                        this->secondary_units.push_back(node);
                    }
                },
                unit);
        }
    }
}

void elaborator::resolve_references() {
    instantiated_entities.clear();
    reference_resolver(*this).run();
    linker(*this).run();
}

std::vector<ast::entity_declaration*> elaborator::get_top_modules() const {
    std::vector<ast::entity_declaration*> result;
    std::unordered_set<ast::entity_declaration*> seen;

    for(const auto& unit : primary_units) {
        std::visit(
            [this, &result, &seen](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::entity_declaration>) {
                    if(instantiated_entities.find(node) == instantiated_entities.end() && seen.insert(node).second)
                        result.push_back(node);
                }
            },
            unit);
    }

    return result;
}
} // namespace vhdl_fe
