// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include <ast_nodes.h>
#include <mutex>
#include <parser.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vhdl_fe {

struct elaboration_diagnostic {
    enum class severity_e { WARNING, ERROR };

    severity_e severity{severity_e::ERROR};
    std::string message;
};

struct elaborator {
    elaborator(parser::parser& p);

    void add_design_files(std::vector<ast::design_file*> const&);
    void resolve_references();
    std::vector<ast::entity_declaration*> get_top_modules() const;
    const std::vector<elaboration_diagnostic>& get_diagnostics() const;

private:
    friend struct reference_resolver;
    friend struct linker;
    void populate_std_packages();
    void add_diagnostic(elaboration_diagnostic::severity_e severity, std::string message);
    // void link_primary_and_secondary_units();

    ast::entity_declaration* find_entity(const std::string& lib_name, const std::string& name);
    ast::architecture_body* find_architecture(const std::string& lib_name, const std::string& entity_name,
                                              const std::string& architecture_name);
    std::vector<ast::architecture_body*> find_architectures_for_entity(const std::string& lib_name, const std::string& entity_name);
    ast::configuration_declaration* find_configuration(const std::string& lib_name, const std::string& name);
    ast::package_declaration* find_package(const std::string& lib_name, const std::string& name);
    ast::context_declaration* find_context(const std::string& lib_name, const std::string& name);

    parser::parser& parser;

    std::unordered_map<std::string, std::vector<ast::design_file*>> files_by_lib;
    std::vector<ast::unit_item> primary_units;
    std::vector<ast::unit_item> secondary_units;

    std::unordered_map<std::string, ast::entity_declaration*> entities_by_key;
    std::unordered_map<std::string, std::vector<ast::architecture_body*>> architectures_by_entity_key;
    std::unordered_map<std::string, ast::architecture_body*> architectures_by_key;
    std::unordered_map<std::string, ast::configuration_declaration*> configurations_by_key;
    std::unordered_map<std::string, ast::package_declaration*> packages_by_key;
    std::unordered_map<std::string, ast::package_instantiation_declaration*> package_instances_by_key;
    std::unordered_map<std::string, ast::package_body*> package_bodies_by_key;
    std::unordered_map<std::string, ast::context_declaration*> contexts_by_key;

    std::unordered_set<ast::entity_declaration*> instantiated_entities;
    std::vector<elaboration_diagnostic> diagnostics;
    std::mutex files_mtx;
};
} // namespace vhdl_fe
