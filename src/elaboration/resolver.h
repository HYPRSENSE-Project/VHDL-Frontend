#include "ast_nodes.h"
#include "elaborator.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <variant>

namespace vhdl_fe {
namespace {

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::string trim(std::string_view value) {
    size_t first = 0;
    while(first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
        ++first;
    size_t last = value.size();
    while(last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
        --last;
    return std::string(value.substr(first, last - first));
}

std::string canonicalize(std::string_view value) {
    auto out = trim(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::vector<std::string> split_name(std::string_view name) {
    std::vector<std::string> parts;
    std::string text(name);
    size_t begin = 0;
    while(begin <= text.size()) {
        auto end = text.find('.', begin);
        auto part = canonicalize(std::string_view(text).substr(begin, end == std::string::npos ? std::string::npos : end - begin));
        if(!part.empty())
            parts.emplace_back(std::move(part));
        if(end == std::string::npos)
            break;
        begin = end + 1;
    }
    return parts;
}

std::string make_key(const std::string& lib, const std::string& name) { return canonicalize(lib) + "." + canonicalize(name); }

std::string make_arch_key(const std::string& lib, const std::string& entity_name, const std::string& architecture_name) {
    return canonicalize(lib) + "." + canonicalize(entity_name) + "." + canonicalize(architecture_name);
}

std::string file_lib(const ast::design_file* file) { return file && !file->lib_name.empty() ? canonicalize(file->lib_name) : "work"; }

std::pair<std::string, std::string> library_and_design_name(const std::string& lib_name, const std::string& name) {
    auto parts = split_name(name);
    if(parts.empty())
        return {canonicalize(lib_name), {}};
    if(parts.size() >= 2)
        return {parts[0], parts[1]};
    return {canonicalize(lib_name), parts[0]};
}

std::string full_selected_name(const ast::used_package* used) {
    if(!used)
        return {};
    std::ostringstream oss;
    oss << used->identifier;
    for(const auto& suffix : used->suffixes)
        oss << '.' << suffix;
    return oss.str();
}

std::string full_selected_name(const ast::selected_name& selected) {
    if(selected.suffix.empty())
        return selected.identifier;
    return selected.identifier + "." + selected.suffix;
}

bool is_all_name(const std::string& value) { return canonicalize(value) == "all"; }
bool is_others_name(const std::string& value) { return canonicalize(value) == "others"; }

bool looks_like_literal(const std::string& text) {
    auto value = canonicalize(text);
    if(value.empty())
        return true;
    if(value == "null")
        return true;
    if(std::isdigit(static_cast<unsigned char>(value.front())))
        return true;
    return value.find('"') != std::string::npos || value.find('\'') != std::string::npos;
}

std::string name_text(const ast::name_node* node) {
    // TODO: implement full name to string conversion
    return node ? node->text : std::string();
}

std::string type_mark_text(const ast::type_mark* node) {
    if(node)
        return node->name->text;
    return std::string();
}

template <typename T> T* ref_as(const ast::declaration_ref& ref) {
    if(auto ptr = std::get_if<T*>(&ref))
        return *ptr;
    return nullptr;
}

bool empty_ref(const ast::declaration_ref& ref) { return std::holds_alternative<std::monostate>(ref); }

template <typename T> void append_unique(std::vector<T*>& values, T* value) {
    if(!value)
        return;
    if(std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

} // namespace

struct reference_resolver {
    struct scope {
        scope* parent{nullptr};
        std::string lib{"work"};
        std::vector<ast::use_clause*> use_clauses;
        std::vector<ast::configuration_specification*> configuration_specs;
        std::unordered_map<std::string, std::vector<ast::declaration_ref>> symbols;
        scope(scope* parent)
        : parent(parent)
        , lib(parent->lib) {}
        scope(std::string const& lib_name)
        : lib(lib_name) {}
        scope() = default;
    };

    elaborator& elab;

    explicit reference_resolver(elaborator& elab_)
    : elab(elab_) {}

    void run() {
        for(const auto& unit : elab.primary_units) {
            std::visit(
                [this](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::package_declaration>)
                        resolve_package(node);
                },
                unit);
        }
        for(const auto& unit : elab.primary_units) {
            std::visit(
                [this](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::context_declaration>)
                        resolve_context(node);
                },
                unit);
        }
        for(const auto& unit : elab.primary_units) {
            std::visit(
                [this](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::entity_declaration>)
                        resolve_entity(node);
                    else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>)
                        resolve_package_instantiation(node);
                    else if constexpr(std::is_same_v<T, ast::configuration_declaration>)
                        resolve_configuration(node);
                    else if constexpr(std::is_same_v<T, ast::context_reference>)
                        resolve_context_reference(node, "work");
                },
                unit);
        }
        for(const auto& unit : elab.secondary_units) {
            std::visit(
                [this](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::architecture_body>)
                        resolve_architecture(node);
                    else if constexpr(std::is_same_v<T, ast::package_body>)
                        resolve_package_body(node);
                },
                unit);
        }
    }

    void insert(scope& sc, const std::string& name, ast::declaration_ref ref) {
        const auto key = canonicalize(name);
        if(key.empty() || empty_ref(ref))
            return;
        sc.symbols[key].push_back(ref);
    }

    ast::declaration_ref lookup(scope& sc, const std::string& name) {
        auto parts = split_name(name);
        if(parts.empty())
            return {};
        if(parts.size() > 1)
            return lookup_qualified(sc.lib, name);
        const auto key = parts[0];
        for(scope* current = &sc; current; current = current->parent) {
            auto it = current->symbols.find(key);
            if(it != current->symbols.end() && !it->second.empty()) {
                if(it->second.size() > 1) {
                    elab.add_diagnostic(elaboration_diagnostic::severity_e::ERROR, "ambiguous reference: " + name);
                    return {};
                }
                return it->second.front();
            }
        }
        if(auto imported = lookup_in_use_clauses(sc, key); !empty_ref(imported))
            return imported;
        return {};
    }

    ast::declaration_ref lookup_qualified(const std::string& lib_name, const std::string& name) {
        auto parts = split_name(name);
        if(parts.empty() || parts.size() == 1) {
            return {};
        }
        if(parts.size() == 2) {
            if(auto* entity = elab.find_entity(lib_name, name))
                return entity;
            if(auto* package = elab.find_package(lib_name, name))
                return package;
            if(auto* context = elab.find_context(lib_name, name))
                return context;
            if(auto* config = elab.find_configuration(lib_name, name))
                return config;
            return {};
        }
        if(parts.size() >= 3) {
            const auto package_name = parts[0] + "." + parts[1];
            if(auto* package = elab.find_package(lib_name, package_name)) {
                if(is_all_name(parts[2]))
                    return package;
                return lookup_in_package(package, parts[2]);
            }
        }
        return {};
    }

    ast::declaration_ref lookup_in_use_clauses(scope& sc, const std::string& name) {
        for(scope* current = &sc; current; current = current->parent) {
            for(auto* use : current->use_clauses) {
                if(!use)
                    continue;
                resolve_use_clause(use, current->lib);
                for(auto* clause : use->clauses) {
                    if(!clause)
                        continue;
                    if(!clause->suffixes.empty() && is_all_name(clause->suffixes.back()) && clause->package_ref) {
                        auto ref = lookup_in_package(clause->package_ref, name);
                        if(!empty_ref(ref))
                            return ref;
                    } else if(!empty_ref(clause->selected_ref)) {
                        const auto parts = split_name(full_selected_name(clause));
                        if(!parts.empty() && parts.back() == name)
                            return clause->selected_ref;
                    }
                }
            }
        }
        return {};
    }

    ast::declaration_ref lookup_in_package(ast::package_declaration* package, const std::string& name) {
        if(!package)
            return {};
        scope package_scope;
        package_scope.lib = file_lib(package->my_file);
        insert_interfaces(package_scope, package->generic_list);
        insert_declarations(package_scope, package->declarations);
        auto it = package_scope.symbols.find(canonicalize(name));
        if(it == package_scope.symbols.end() || it->second.empty())
            return {};
        if(it->second.size() > 1)
            return {};
        return it->second.front();
    }

    void resolve_use_clause(ast::use_clause* use, const std::string& lib_name) {
        if(!use)
            return;
        for(auto* clause : use->clauses) {
            if(!clause || clause->package_ref || !std::holds_alternative<std::monostate>(clause->selected_ref))
                continue;
            auto parts = split_name(full_selected_name(clause));
            if(parts.empty())
                continue;
            if(parts.size() >= 3) {
                if(is_all_name(parts.back())) {
                    clause->package_ref = elab.find_package(lib_name, parts[0] + "." + parts[1]);
                    continue;
                } else {
                    clause->package_ref = elab.find_package(lib_name, parts[0] + "." + parts[1]);
                    if(clause->package_ref)
                        clause->selected_ref = lookup_in_package(clause->package_ref, parts[2]);
                    continue;
                }
            }
            if(parts.size() == 2) {
                clause->selected_ref = lookup_qualified(lib_name, full_selected_name(clause));
            }
        }
    }

    void resolve_context_reference(ast::context_reference* ref, const std::string& lib_name) {
        if(!ref)
            return;
        for(auto& selected : ref->selected_names) {
            if(auto* context = elab.find_context(lib_name, full_selected_name(selected)))
                selected.resolved_ref = context;
        }
    }

    void apply_unit_use_clauses(scope& sc, const std::vector<ast::use_clause*>& clauses) {
        sc.use_clauses = clauses;
        auto use_std = elab.parser.anf.create<ast::use_clause>();
        use_std->clauses.emplace_back(elab.parser.anf.create<ast::used_package>());
        use_std->clauses.back()->identifier = "std";
        use_std->clauses.back()->suffixes.emplace_back("standard");
        use_std->clauses.back()->suffixes.emplace_back("all");
        sc.use_clauses.push_back(use_std);
        for(auto* use : sc.use_clauses)
            resolve_use_clause(use, sc.lib);
        // auto package_ref = elab.find_package("std", "std.standard");
        // if(package_ref)
        //     insert_declarations(sc, package_ref->declarations);
        // for(auto* use : sc.use_clauses) {
        //     resolve_use_clause(use, sc.lib);
        //     for(auto* clause : use->clauses) {
        //         // TODO: insert all declarations from clause->selected_ref or clause->package_ref into scope
        //     }
        // }
    }

    void resolve_entity(ast::entity_declaration* entity) {
        scope sc;
        sc.lib = file_lib(entity->my_file);
        apply_unit_use_clauses(sc, entity->packages_in_scope);
        insert_interfaces(sc, entity->generic_list);
        insert_interfaces(sc, entity->port_list);
        insert_declarations(sc, entity->entity_declarative_items);
        resolve_interfaces(sc, entity->generic_list);
        resolve_interfaces(sc, entity->port_list);
        resolve_declarations(sc, entity->entity_declarative_items);
        for(auto* stmt : entity->entity_statements)
            resolve_entity_statement(sc, stmt);
    }

    void resolve_architecture(ast::architecture_body* arch) {
        scope root_sc(file_lib(arch->my_file));
        apply_unit_use_clauses(root_sc, arch->packages_in_scope);
        arch->primary_ref = elab.find_entity(root_sc.lib, name_text(arch->primary));
        scope prim_sc(&root_sc);
        if(arch->primary_ref) {
            arch->primary_ref->architectures.push_back(arch);
            insert_interfaces(prim_sc, arch->primary_ref->generic_list);
            insert_interfaces(prim_sc, arch->primary_ref->port_list);
            insert_declarations(prim_sc, arch->primary_ref->entity_declarative_items);
        }
        scope sc(&prim_sc);
        insert_declarations(sc, arch->block_declarative_items);
        resolve_declarations(sc, arch->block_declarative_items);
        for(auto* stmt : arch->concurrent_statements)
            resolve_concurrent_statement(sc, stmt);
    }

    void resolve_package(ast::package_declaration* package) {
        scope root_scope(file_lib(package->my_file));
        apply_unit_use_clauses(root_scope, package->packages_in_scope);
        scope sc(&root_scope);
        insert_interfaces(sc, package->generic_list);
        insert_declarations(sc, package->declarations);
        resolve_interfaces(sc, package->generic_list);
        resolve_associations(sc, package->generic_map, sc, nullptr);
        resolve_declarations(sc, package->declarations);
    }

    void resolve_package_body(ast::package_body* body) {
        scope root_scope(file_lib(body->my_file));
        apply_unit_use_clauses(root_scope, body->packages_in_scope);
        scope sc(&root_scope);
        body->package_ref = elab.find_package(sc.lib, body->identifier);
        if(body->package_ref) {
            body->package_ref->body.push_back(body);
            insert_interfaces(sc, body->package_ref->generic_list);
            insert_declarations(sc, body->package_ref->declarations);
        }
        insert_declarations(sc, body->declarative_items);
        resolve_declarations(sc, body->declarative_items);
    }

    void resolve_package_instantiation(ast::package_instantiation_declaration* inst) {
        scope sc;
        sc.lib = file_lib(inst->my_file);
        apply_unit_use_clauses(sc, inst->packages_in_scope);
        inst->target_package_ref = elab.find_package(sc.lib, name_text(inst->target_name));
        scope formal_scope;
        formal_scope.lib = sc.lib;
        if(inst->target_package_ref)
            insert_interfaces(formal_scope, inst->target_package_ref->generic_list);
        resolve_associations(sc, inst->generic_map, sc, &formal_scope);
    }

    void resolve_configuration(ast::configuration_declaration* config) {
        scope sc;
        sc.lib = file_lib(config->my_file);
        apply_unit_use_clauses(sc, config->packages_in_scope);
        config->entity_ref = elab.find_entity(sc.lib, config->name);
        insert_declarations(sc, config->declarative_items);
        resolve_declarations(sc, config->declarative_items);
        resolve_block_configuration(sc, config->block_config);
    }

    void resolve_context(ast::context_declaration* context) {
        const auto lib = file_lib(context->my_file);
        for(auto& item : context->context_items) {
            std::visit(overloaded{
                           [](ast::library_clause*) {},
                           [this, &lib](ast::use_clause* use) { resolve_use_clause(use, lib); },
                           [this, &lib](ast::context_reference* ref) { resolve_context_reference(ref, lib); },
                       },
                       item);
        }
    }

    template <typename Vector> void insert_interfaces(scope& sc, Vector& interfaces) {
        for(auto& item : interfaces)
            insert_interface(sc, item);
    }

    void insert_interface(scope& sc, ast::interface_declaration_item& item) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::interface_constant_declaration> ||
                             std::is_same_v<T, ast::interface_signal_declaration> ||
                             std::is_same_v<T, ast::interface_variable_declaration> || std::is_same_v<T, ast::interface_file_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::interface_type_declaration> ||
                                    std::is_same_v<T, ast::interface_package_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::interface_subprogram_declaration>) {
                    std::visit(
                        [this, &sc, node](auto* spec) {
                            if(spec)
                                insert(sc, spec->designator, node);
                        },
                        node->nterface_subprogram_specification);
                }
            },
            item);
    }

    template <typename Vector> void insert_declarations(scope& sc, Vector& declarations) {
        for(auto& item : declarations)
            insert_declaration(sc, item);
    }

    template <typename Variant> void insert_declaration(scope& sc, Variant& item) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::signal_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::component_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::subprogram_declaration>) {
                    auto it = sc.symbols.find(canonicalize(node->designator));
                    if(it != sc.symbols.end()) {
                        auto& res = it->second;
                        if(res.size() == 1) {
                            if(std::holds_alternative<ast::subprogram_declaration_overload*>(res[0])) {
                                std::get<ast::subprogram_declaration_overload*>(res[0])->overloads.push_back(node);
                                return;
                            } else if(std::holds_alternative<ast::subprogram_declaration*>(res[0])) {
                                auto decl = std::get<ast::subprogram_declaration*>(res[0]);
                                auto overload = elab.parser.anf.create<ast::subprogram_declaration_overload>();
                                overload->designator = node->designator;
                                overload->overloads.push_back(decl);
                                res.clear();
                                res.push_back(overload);
                                return;
                            }
                        }
                    }
                    insert(sc, node->designator, node);
                } else if constexpr(std::is_same_v<T, ast::subprogram_instantiation_declaration>) {
                    insert(sc, node->designator, node);
                } else if constexpr(std::is_same_v<T, ast::subprogram_body>) {
                    if(node->specification)
                        insert(sc, node->specification->designator, node->specification);
                } else if constexpr(std::is_same_v<T, ast::package_declaration> ||
                                    std::is_same_v<T, ast::package_instantiation_declaration> || std::is_same_v<T, ast::type_declaration> ||
                                    std::is_same_v<T, ast::subtype_declaration> || std::is_same_v<T, ast::attribute_declaration> ||
                                    std::is_same_v<T, ast::group_template_declaration> || std::is_same_v<T, ast::group_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::constant_declaration> || std::is_same_v<T, ast::variable_declaration> ||
                                    std::is_same_v<T, ast::file_declaration>) {
                    insert(sc, node->identifier, node);
                } else if constexpr(std::is_same_v<T, ast::alias_declaration>) {
                    insert(sc, node->alias_designator, node);
                } else if constexpr(std::is_same_v<T, ast::configuration_specification>) {
                    sc.configuration_specs.push_back(node);
                }
            },
            item);
    }

    template <typename Vector> void resolve_interfaces(scope& sc, Vector& interfaces) {
        for(auto& item : interfaces)
            resolve_interface(sc, item);
    }

    void resolve_interface(scope& sc, ast::interface_declaration_item& item) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::interface_constant_declaration> ||
                             std::is_same_v<T, ast::interface_signal_declaration> ||
                             std::is_same_v<T, ast::interface_variable_declaration> || std::is_same_v<T, ast::interface_file_declaration>) {
                    resolve_subtype_indication(sc, node->subtype_indic);
                    if constexpr(!std::is_same_v<T, ast::interface_file_declaration>)
                        resolve_expression(sc, node->expression);
                } else if constexpr(std::is_same_v<T, ast::interface_subprogram_declaration>) {
                    std::visit(
                        [this, &sc](auto* spec) {
                            if(!spec)
                                return;
                            using S = std::decay_t<decltype(*spec)>;
                            if constexpr(std::is_same_v<S, ast::interface_procedure_specification>) {
                                resolve_interfaces(sc, spec->formal_parameter_list);
                            } else {
                                resolve_interfaces(sc, spec->formal_parameter_list);
                                spec->return_type_ref = lookup(sc, type_mark_text(spec->return_type_mark));
                            }
                        },
                        node->nterface_subprogram_specification);
                } else if constexpr(std::is_same_v<T, ast::interface_package_declaration>) {
                    node->package_ref = elab.find_package(sc.lib, name_text(node->name));
                    resolve_associations(sc, node->generic_map_aspect, sc, nullptr);
                }
            },
            item);
    }

    template <typename Vector> void resolve_declarations(scope& sc, Vector& declarations) {
        for(auto& item : declarations)
            resolve_declaration(sc, item);
    }

    template <typename Variant> void resolve_declaration(scope& sc, Variant& item) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::signal_declaration>) {
                    node->type_ref = lookup(sc, type_mark_text(node->type));
                    resolve_resolution(sc, node->resolution);
                } else if constexpr(std::is_same_v<T, ast::component_declaration>) {
                    resolve_interfaces(sc, node->generic_list);
                    resolve_interfaces(sc, node->port_list);
                } else if constexpr(std::is_same_v<T, ast::configuration_specification>) {
                    resolve_component_specification(sc, node->component_spec);
                    resolve_binding(sc, node->binding);
                } else if constexpr(std::is_same_v<T, ast::type_declaration>) {
                    resolve_type_definition(sc, node->type);
                } else if constexpr(std::is_same_v<T, ast::subtype_declaration>) {
                    resolve_subtype_indication(sc, node->indication);
                } else if constexpr(std::is_same_v<T, ast::constant_declaration>) {
                    resolve_subtype_indication(sc, node->indication);
                    resolve_expression(sc, node->expr);
                } else if constexpr(std::is_same_v<T, ast::variable_declaration>) {
                    resolve_subtype_indication(sc, node->indication);
                    resolve_expression(sc, node->expr);
                } else if constexpr(std::is_same_v<T, ast::file_declaration>) {
                    resolve_subtype_indication(sc, node->indication);
                    resolve_expression(sc, node->open_expression);
                    resolve_expression(sc, node->file_logical_name);
                } else if constexpr(std::is_same_v<T, ast::alias_declaration>) {
                    resolve_subtype_indication(sc, node->indication);
                    node->name_ref = lookup(sc, node->name);
                    node->type_mark_refs.clear();
                    for(const auto& type_mark : node->type_marks)
                        node->type_mark_refs.push_back(lookup(sc, type_mark_text(type_mark)));
                    node->return_type_mark_ref = lookup(sc, type_mark_text(node->return_type_mark));
                } else if constexpr(std::is_same_v<T, ast::attribute_declaration>) {
                    node->type_ref = lookup(sc, type_mark_text(node->type));
                } else if constexpr(std::is_same_v<T, ast::attribute_specification>) {
                    node->entity_refs.clear();
                    for(const auto& name : node->entity_name_list)
                        node->entity_refs.push_back(lookup(sc, name));
                    resolve_expression(sc, node->expr);
                } else if constexpr(std::is_same_v<T, ast::group_declaration>) {
                    node->template_ref = lookup(sc, node->name);
                    node->constituent_refs.clear();
                    for(const auto& name : node->group_constituent_list)
                        node->constituent_refs.push_back(lookup(sc, name));
                } else if constexpr(std::is_same_v<T, ast::disconnection_specification>) {
                    node->signal_ref = lookup(sc, node->signal_name);
                    node->type_ref = lookup(sc, type_mark_text(node->type));
                    resolve_expression(sc, node->after_expression);
                } else if constexpr(std::is_same_v<T, ast::subprogram_declaration>) {
                    resolve_subprogram_declaration(sc, node);
                } else if constexpr(std::is_same_v<T, ast::subprogram_instantiation_declaration>) {
                    node->target_ref = ref_as<ast::subprogram_declaration>(lookup(sc, name_text(node->target_name)));
                } else if constexpr(std::is_same_v<T, ast::subprogram_body>) {
                    if(node->specification)
                        resolve_subprogram_declaration(sc, node->specification);
                    scope child{&sc};
                    insert_declarations(child, node->declarative_items);
                    resolve_declarations(child, node->declarative_items);
                    for(auto* stmt : node->sequential_statements)
                        resolve_sequential_statement(child, stmt);
                } else if constexpr(std::is_same_v<T, ast::package_declaration>) {
                    resolve_package(node);
                } else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>) {
                    resolve_package_instantiation(node);
                } else if constexpr(std::is_same_v<T, ast::package_body>) {
                    resolve_package_body(node);
                }
            },
            item);
    }

    void resolve_subprogram_declaration(scope& sc, ast::subprogram_declaration* subprogram) {
        if(!subprogram)
            return;
        resolve_interfaces(sc, subprogram->generic_list);
        resolve_associations(sc, subprogram->generic_map, sc, nullptr);
        resolve_interfaces(sc, subprogram->parameter_list);
        subprogram->return_type_ref = lookup(sc, type_mark_text(subprogram->return_type));
    }

    void resolve_type_definition(scope& sc, ast::type_definition_item& type) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::numeric_type_definition>) {
                    std::visit([this, &sc](auto* range) { resolve_constraint_node(sc, range); }, node->range);
                } else if constexpr(std::is_same_v<T, ast::constrained_array_definition>) {
                    for(auto& constraint : node->index_constraints)
                        resolve_discrete_range(sc, constraint);
                    resolve_subtype_indication(sc, node->subtype_indication_);
                } else if constexpr(std::is_same_v<T, ast::record_type_definition>) {
                    for(auto* elem : node->element_declarations)
                        if(elem)
                            resolve_subtype_indication(sc, elem->element_subtype_definition);
                } else if constexpr(std::is_same_v<T, ast::subtype_indication>) {
                    resolve_subtype_indication(sc, node);
                } else if constexpr(std::is_same_v<T, ast::literal_node>) {
                    if(!looks_like_literal(node->text))
                        node->resolved_ref = lookup(sc, node->text);
                } else if constexpr(std::is_same_v<T, ast::protected_type_definition>) {
                    resolve_declarations(sc, node->declarations);
                    resolve_declarations(sc, node->body_declarations);
                } else if constexpr(std::is_same_v<T, ast::protected_type_declaration>) {
                    resolve_declarations(sc, node->protected_type_declarative_items);
                }
            },
            type);
    }

    ast::declaration_ref lookup(scope& sc, ast::name_node* name) {
        if(!name)
            return {};
        switch(name->kind) {
        default:
            return lookup(sc, name->value);
        case ast::name_kind_e::SLICE:
            return lookup(sc, name->prefix->value);
        }
    }

    void resolve_type_mark(scope& sc, ast::type_mark* mark) {
        if(!mark)
            return;
        mark->resolved_ref = lookup(sc, mark->name);
    }

    void resolve_subtype_indication(scope& sc, ast::subtype_indication* subtype) {
        if(!subtype)
            return;
        resolve_resolution(sc, subtype->resolution);
        resolve_type_mark(sc, subtype->type);
        resolve_constraint(sc, subtype->constr);
    }

    void resolve_resolution(scope& sc, ast::resolution_indication* resolution) {
        if(!resolution)
            return;
        resolution->resolution_ref = lookup(sc, resolution->name);
        std::visit(
            [this, &sc](auto* item) {
                if constexpr(std::is_same_v<std::decay_t<decltype(item)>, ast::resolution_indication*>)
                    resolve_resolution(sc, item);
            },
            resolution->elem_resolution);
    }

    void resolve_constraint(scope& sc, ast::constraint_item& constraint) {
        std::visit([this, &sc](auto* node) { resolve_constraint_node(sc, node); }, constraint);
    }

    void resolve_constraint_node(scope& sc, ast::explicit_range* range) { resolve_range(sc, range); }
    void resolve_constraint_node(scope& sc, ast::attribute_range* range) {
        if(range)
            range->prefix_ref = lookup(sc, range->name);
    }
    void resolve_constraint_node(scope& sc, ast::array_constraint* constraint) {
        if(!constraint)
            return;
        for(auto& item : constraint->index_constraint)
            resolve_discrete_range(sc, item);
    }
    void resolve_constraint_node(scope& sc, ast::record_constraint* constraint) {
        if(!constraint)
            return;
        for(auto& elem : constraint->constraint)
            resolve_subtype_indication(sc, elem.indication);
    }

    void resolve_discrete_range(scope& sc, ast::discrete_range_item& item) {
        std::visit(
            [this, &sc](auto* node) {
                if constexpr(std::is_same_v<std::decay_t<decltype(node)>, ast::subtype_indication*>)
                    resolve_subtype_indication(sc, node);
                else
                    resolve_constraint_node(sc, node);
            },
            item);
    }

    void resolve_range(scope& sc, ast::explicit_range* range) {
        if(!range)
            return;
        resolve_expression(sc, range->left);
        resolve_expression(sc, range->right);
    }

    void resolve_expression(scope& sc, ast::expression_item& expression) {
        std::visit(
            [this, &sc](auto* node) {
                if(node)
                    resolve_expression_node(sc, node);
            },
            expression);
    }

    void resolve_primary(scope& sc, ast::primary_item& primary) {
        std::visit(
            [this, &sc](auto* node) {
                if(node)
                    resolve_expression_node(sc, node);
            },
            primary);
    }

    void resolve_expression_node(scope& sc, ast::literal_node* node) {
        if(!node || looks_like_literal(node->text))
            return;
        node->resolved_ref = lookup(sc, node->text);
    }
    void resolve_expression_node(scope& sc, ast::name_node* node) {
        if(!node || !node->text.size())
            return;
        if(node->text.at(0) == '"' || node->text.at(0) == '\'') // name is a character or a string
            return;
        node->resolved_ref = lookup(sc, node->text);
    }
    void resolve_expression_node(scope& sc, ast::allocator* node) {
        if(!node)
            return;
        resolve_subtype_indication(sc, node->si);
        resolve_expression_node(sc, node->qe);
    }
    void resolve_expression_node(scope& sc, ast::aggregate* node) {
        if(!node)
            return;
        for(auto* assoc : node->elem_assoc) {
            if(!assoc)
                continue;
            for(auto* choice : assoc->choices)
                resolve_expression_node(sc, choice);
            resolve_expression(sc, assoc->expr);
        }
    }
    void resolve_expression_node(scope& sc, ast::qualified_expression* node) {
        if(!node)
            return;
        node->type_ref = lookup(sc, node->type->name->text);
        resolve_expression_node(sc, node->aggr);
    }
    void resolve_expression_node(scope& sc, ast::simple_expression* node) {
        if(!node)
            return;
        resolve_primary(sc, node->first_primary);
        resolve_primary(sc, node->secondary_primary);
        resolve_expression_node(sc, node->lhs);
        resolve_expression_node(sc, node->rhs);
        resolve_expression_node(sc, node->operand);
    }
    void resolve_expression_node(scope& sc, ast::conditional_primary* node) {
        if(node)
            resolve_primary(sc, node->prim);
    }
    void resolve_expression_node(scope& sc, ast::binary_expression* node) {
        if(!node)
            return;
        resolve_expression(sc, node->lhs);
        resolve_expression(sc, node->rhs);
    }

    void resolve_target(scope& sc, ast::target_item& target) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::aggregate>)
                    resolve_expression_node(sc, node);
                else if constexpr(std::is_same_v<T, ast::literal_node>)
                    node->resolved_ref = lookup(sc, node->text);
            },
            target);
    }

    void resolve_associations(scope& current_scope, std::vector<ast::association_element*>& associations, scope& actual_scope,
                              scope* formal_scope) {
        for(auto* assoc : associations) {
            if(!assoc)
                continue;
            if(formal_scope && !assoc->formal_name.empty())
                assoc->formal_ref = lookup(*formal_scope, assoc->formal_name);
            if(!assoc->actual_name.empty())
                assoc->actual_ref = lookup(actual_scope, assoc->actual_name);
            if(assoc->act_designator) {
                resolve_expression(current_scope, assoc->act_designator->expr);
                resolve_subtype_indication(current_scope, assoc->act_designator->indication);
            }
        }
    }

    scope make_interface_scope(const std::string& lib, ast::entity_declaration* entity, ast::component_declaration* component) {
        scope formal;
        formal.lib = lib;
        if(entity) {
            insert_interfaces(formal, entity->generic_list);
            insert_interfaces(formal, entity->port_list);
        }
        if(component) {
            insert_interfaces(formal, component->generic_list);
            insert_interfaces(formal, component->port_list);
        }
        return formal;
    }

    void resolve_component_specification(scope& sc, ast::component_specification* spec) {
        if(!spec)
            return;
        spec->component_ref = ref_as<ast::component_declaration>(lookup(sc, spec->name));
    }

    void resolve_binding(scope& sc, ast::binding_indication* binding) {
        if(!binding || binding->type == ast::entity_aspect_e::OPEN)
            return;

        if(binding->type == ast::entity_aspect_e::ENTITY) {
            binding->entity_ref = elab.find_entity(sc.lib, binding->unit_ref);
            binding->architecture_ref = elab.find_architecture(sc.lib, binding->unit_ref, binding->identifier);
            scope formal = make_interface_scope(sc.lib, binding->entity_ref, nullptr);
            resolve_associations(sc, binding->generic_map, sc, &formal);
            resolve_associations(sc, binding->port_map, sc, &formal);
        } else if(binding->type == ast::entity_aspect_e::CONFIGURATION) {
            binding->configuration_ref = elab.find_configuration(sc.lib, binding->unit_ref);
            if(binding->configuration_ref)
                binding->entity_ref = binding->configuration_ref->entity_ref;
        }
    }

    ast::configuration_specification* find_matching_configuration(scope& sc, const std::string& label, const std::string& component_name) {
        const auto label_key = canonicalize(label);
        const auto comp_key = canonicalize(component_name);
        for(scope* current = &sc; current; current = current->parent) {
            for(auto* config : current->configuration_specs) {
                if(!config || !config->component_spec)
                    continue;
                if(canonicalize(config->component_spec->name) != comp_key)
                    continue;
                for(const auto& instantiation : config->component_spec->instantiations) {
                    const auto inst_key = canonicalize(instantiation);
                    if(inst_key == label_key || is_all_name(inst_key) || is_others_name(inst_key))
                        return config;
                }
            }
        }
        return nullptr;
    }

    void resolve_component_instantiation(scope& sc, const std::string& label, ast::component_instantiation_statement* inst) {
        if(!inst)
            return;

        if(inst->unit_kind == ast::instantiated_unit_kind_e::ENTITY) {
            inst->entity_ref = elab.find_entity(sc.lib, inst->unit_name);
            inst->architecture_ref = elab.find_architecture(sc.lib, inst->unit_name, inst->architecture_id);
        } else if(inst->unit_kind == ast::instantiated_unit_kind_e::CONFIGURATION) {
            inst->configuration_ref = elab.find_configuration(sc.lib, inst->unit_name);
            if(inst->configuration_ref)
                inst->entity_ref = inst->configuration_ref->entity_ref;
        } else {
            inst->component_ref = ref_as<ast::component_declaration>(lookup(sc, inst->unit_name));
            if(auto* config = find_matching_configuration(sc, label, inst->unit_name)) {
                resolve_binding(sc, config->binding);
                if(config->binding) {
                    inst->entity_ref = config->binding->entity_ref;
                    inst->architecture_ref = config->binding->architecture_ref;
                    inst->configuration_ref = config->binding->configuration_ref;
                }
            }
            // TODO: this needs to be done in a separate linking step
            // if(!inst->entity_ref && inst->component_ref) {
            //     inst->entity_ref = elab.find_entity(sc.lib, inst->component_ref->identifier);
            //     inst->architecture_ref = elab.find_architecture(sc.lib, inst->component_ref->identifier, {});
            // }
        }

        if(inst->configuration_ref && !inst->entity_ref)
            inst->entity_ref = inst->configuration_ref->entity_ref;
        if(inst->entity_ref)
            elab.instantiated_entities.insert(inst->entity_ref);

        scope formal = make_interface_scope(sc.lib, inst->entity_ref, inst->component_ref);
        resolve_associations(sc, inst->generic_map, sc, &formal);
        resolve_associations(sc, inst->port_map, sc, &formal);
    }

    void resolve_block_configuration(scope& sc, ast::block_configuration* block) {
        if(!block)
            return;
        scope child{&sc};
        child.use_clauses = block->use_clauses;
        for(auto* use : child.use_clauses)
            resolve_use_clause(use, child.lib);
        for(auto& item : block->configuration_items) {
            std::visit(
                [this, &child](auto* node) {
                    if(!node)
                        return;
                    using T = std::decay_t<decltype(*node)>;
                    if constexpr(std::is_same_v<T, ast::block_configuration>) {
                        resolve_block_configuration(child, node);
                    } else if constexpr(std::is_same_v<T, ast::component_configuration>) {
                        resolve_component_specification(child, node->component_spec);
                        resolve_binding(child, node->binding);
                        resolve_block_configuration(child, node->block_config);
                    }
                },
                item);
        }
    }

    void resolve_entity_statement(scope& sc, ast::entity_statement* stmt) {
        if(!stmt)
            return;
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::process_statement>)
                    resolve_process(sc, node);
                else if constexpr(std::is_same_v<T, ast::concurrent_procedure_call_statement>)
                    node->procedure_ref = lookup(sc, node->name);
                else if constexpr(std::is_same_v<T, ast::concurrent_assertion_statement>) {
                    resolve_expression(sc, node->condition);
                    resolve_expression(sc, node->report);
                    resolve_expression(sc, node->severity);
                }
            },
            stmt->statement);
    }

    void resolve_concurrent_statement(scope& sc, ast::concurrent_statement* stmt) {
        if(!stmt)
            return;
        std::visit(
            [this, &sc, stmt](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::component_instantiation_statement>) {
                    const auto label = stmt->label.empty() ? node->label : stmt->label;
                    resolve_component_instantiation(sc, label, node);
                } else if constexpr(std::is_same_v<T, ast::block_statement>) {
                    resolve_block_statement(sc, node);
                } else if constexpr(std::is_same_v<T, ast::for_generate_statement>) {
                    resolve_generate_body(sc, node->body);
                } else if constexpr(std::is_same_v<T, ast::if_generate_statement>) {
                    for(auto* clause : node->clauses) {
                        if(!clause)
                            continue;
                        resolve_expression(sc, clause->condition);
                        resolve_generate_body(sc, clause->body);
                    }
                    resolve_generate_body(sc, node->else_body);
                } else if constexpr(std::is_same_v<T, ast::case_generate_statement>) {
                    for(auto* alt : node->alternatives)
                        if(alt)
                            resolve_generate_body(sc, alt->body);
                } else if constexpr(std::is_same_v<T, ast::process_statement>) {
                    resolve_process(sc, node);
                } else if constexpr(std::is_same_v<T, ast::concurrent_procedure_call_statement>) {
                    node->procedure_ref = lookup(sc, node->name);
                } else if constexpr(std::is_same_v<T, ast::concurrent_assertion_statement>) {
                    resolve_expression(sc, node->condition);
                    resolve_expression(sc, node->report);
                    resolve_expression(sc, node->severity);
                } else if constexpr(std::is_same_v<T, ast::concurrent_signal_assignment_any>) {
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->delay_mechanism_reject);
                    for(auto* waveform : node->waveform)
                        resolve_waveform(sc, waveform);
                    for(auto* conditional : node->conditional_waveforms) {
                        if(!conditional)
                            continue;
                        for(auto* waveform : conditional->waveform)
                            resolve_waveform(sc, waveform);
                        resolve_expression(sc, conditional->condition);
                    }
                } else if constexpr(std::is_same_v<T, ast::concurrent_selected_signal_assignment>) {
                    resolve_expression(sc, node->with_expression);
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->delay_mechanism_reject);
                    for(auto* selected : node->selected_waveforms)
                        resolve_selected_waveform(sc, selected);
                }
            },
            stmt->statement);
    }

    void resolve_block_statement(scope& parent, ast::block_statement* block) {
        if(!block)
            return;
        scope sc{&parent};
        resolve_expression(parent, block->condition);
        insert_interfaces(sc, block->generic_list);
        insert_interfaces(sc, block->port_list);
        insert_declarations(sc, block->block_declarative_items);
        resolve_interfaces(sc, block->generic_list);
        resolve_interfaces(sc, block->port_list);
        resolve_associations(sc, block->generic_map, parent, &sc);
        resolve_associations(sc, block->port_map, parent, &sc);
        resolve_declarations(sc, block->block_declarative_items);
        for(auto* stmt : block->concurrent_statements)
            resolve_concurrent_statement(sc, stmt);
    }

    void resolve_generate_body(scope& parent, ast::generate_statement_body& body) {
        scope sc{&parent};
        insert_declarations(sc, body.block_declarative_items);
        resolve_declarations(sc, body.block_declarative_items);
        for(auto* stmt : body.concurrent_statements)
            resolve_concurrent_statement(sc, stmt);
    }

    void resolve_process(scope& parent, ast::process_statement* process) {
        if(!process)
            return;
        scope sc{&parent};
        process->sensitivity_refs.clear();
        for(const auto& name : process->sensitivity_list)
            process->sensitivity_refs.push_back(lookup(parent, name));
        insert_declarations(sc, process->declarative_items);
        resolve_declarations(sc, process->declarative_items);
        for(auto* stmt : process->sequential_statements)
            resolve_sequential_statement(sc, stmt);
    }

    void resolve_waveform(scope& sc, ast::waveform_element* waveform) {
        if(!waveform)
            return;
        resolve_expression(sc, waveform->value);
        resolve_expression(sc, waveform->after_expression);
    }

    void resolve_selected_waveform(scope& sc, ast::selected_waveform_element* selected) {
        if(!selected)
            return;
        for(auto* waveform : selected->waveform)
            resolve_waveform(sc, waveform);
        for(auto& choice : selected->choices)
            resolve_choice(sc, choice);
    }

    void resolve_choice(scope& sc, ast::choice_item& choice) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::subtype_indication>)
                    resolve_subtype_indication(sc, node);
                else if constexpr(std::is_same_v<T, ast::attribute_range> || std::is_same_v<T, ast::explicit_range>)
                    resolve_constraint_node(sc, node);
                else if constexpr(std::is_same_v<T, ast::simple_expression>)
                    resolve_expression_node(sc, node);
                else if constexpr(std::is_same_v<T, ast::literal_node>)
                    resolve_expression_node(sc, node);
            },
            choice);
    }

    void resolve_sequential_statement(scope& sc, ast::sequential_statement* stmt) {
        if(!stmt)
            return;
        resolve_sequential_item(sc, stmt->stmt);
    }

    void resolve_sequential_item(scope& sc, ast::sequential_statement_item& item) {
        std::visit(
            [this, &sc](auto* node) {
                if(!node)
                    return;
                using T = std::decay_t<decltype(*node)>;
                if constexpr(std::is_same_v<T, ast::wait_statement>) {
                    node->sensitivity_refs.clear();
                    for(const auto& name : node->sensitivity_list)
                        node->sensitivity_refs.push_back(lookup(sc, name));
                    resolve_expression(sc, node->condition_clause);
                    resolve_expression(sc, node->timeout_clause);
                } else if constexpr(std::is_same_v<T, ast::assertion_statement>) {
                    resolve_expression(sc, node->condition);
                    resolve_expression(sc, node->report);
                    resolve_expression(sc, node->severity);
                } else if constexpr(std::is_same_v<T, ast::report_statement>) {
                    resolve_expression(sc, node->report);
                    resolve_expression(sc, node->severity);
                } else if constexpr(std::is_same_v<T, ast::simple_waveform_assignment>) {
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->delay_mechanism_reject);
                    for(auto& waveform : node->waveform)
                        resolve_waveform_value(sc, waveform);
                } else if constexpr(std::is_same_v<T, ast::simple_force_assignment>) {
                    resolve_target(sc, node->target);
                    for(auto& waveform : node->waveform)
                        resolve_waveform_value(sc, waveform);
                } else if constexpr(std::is_same_v<T, ast::simple_release_assignment>) {
                    resolve_target(sc, node->target);
                } else if constexpr(std::is_same_v<T, ast::conditional_waveform_assignment>) {
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->delay_mechanism_reject);
                    for(auto& conditional : node->conditional_waveforms)
                        resolve_conditional_waveform_value(sc, conditional);
                } else if constexpr(std::is_same_v<T, ast::conditional_force_assignment>) {
                    resolve_target(sc, node->target);
                    for(auto* conditional : node->conditional_expressions)
                        resolve_conditional_expression(sc, conditional);
                } else if constexpr(std::is_same_v<T, ast::selected_waveform_assignment>) {
                    resolve_expression(sc, node->with_expression);
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->delay_mechanism_reject);
                    for(auto* selected : node->selected_waveforms)
                        resolve_selected_waveform(sc, selected);
                } else if constexpr(std::is_same_v<T, ast::selected_force_assignment>) {
                    resolve_expression(sc, node->with_expression);
                    resolve_target(sc, node->target);
                    for(auto* selected : node->selected_expressions)
                        resolve_selected_expression(sc, selected);
                } else if constexpr(std::is_same_v<T, ast::simple_variable_assignment>) {
                    resolve_target(sc, node->target);
                    resolve_expression(sc, node->value);
                } else if constexpr(std::is_same_v<T, ast::conditional_variable_assignment>) {
                    resolve_target(sc, node->target);
                    for(auto* conditional : node->conditional_expressions)
                        resolve_conditional_expression(sc, conditional);
                } else if constexpr(std::is_same_v<T, ast::selected_variable_assignment>) {
                    resolve_expression(sc, node->with_expression);
                    resolve_target(sc, node->target);
                    for(auto* selected : node->selected_expressions)
                        resolve_selected_expression(sc, selected);
                } else if constexpr(std::is_same_v<T, ast::procedure_call_statement>) {
                    node->procedure_ref = lookup(sc, node->name);
                } else if constexpr(std::is_same_v<T, ast::if_statement>) {
                    for(auto& clause : node->if_clauses) {
                        resolve_expression(sc, std::get<0>(clause));
                        for(auto& nested : std::get<1>(clause))
                            resolve_sequential_item(sc, nested);
                    }
                    for(auto& nested : node->else_clauses)
                        resolve_sequential_item(sc, nested);
                } else if constexpr(std::is_same_v<T, ast::case_statement>) {
                    resolve_expression(sc, node->expression);
                    for(auto& alt : node->alternatives) {
                        for(auto& choice : alt.choices)
                            resolve_choice(sc, choice);
                        for(auto& nested : alt.statements)
                            resolve_sequential_item(sc, nested);
                    }
                } else if constexpr(std::is_same_v<T, ast::loop_statement>) {
                    if(auto* expr = std::get_if<ast::expression_item>(&node->iteration_scheme))
                        resolve_expression(sc, *expr);
                    for(auto& nested : node->statements)
                        resolve_sequential_item(sc, nested);
                } else if constexpr(std::is_same_v<T, ast::next_statement>) {
                    resolve_expression(sc, node->exit_expression);
                } else if constexpr(std::is_same_v<T, ast::exit_statement>) {
                    resolve_expression(sc, node->exit_expression);
                } else if constexpr(std::is_same_v<T, ast::return_statement>) {
                    resolve_expression(sc, node->return_expression);
                }
            },
            item);
    }

    void resolve_waveform_value(scope& sc, ast::waveform_element& waveform) {
        resolve_expression(sc, waveform.value);
        resolve_expression(sc, waveform.after_expression);
    }

    void resolve_conditional_waveform_value(scope& sc, ast::conditional_waveform_element& conditional) {
        for(auto* waveform : conditional.waveform)
            resolve_waveform(sc, waveform);
        resolve_expression(sc, conditional.condition);
    }

    void resolve_conditional_expression(scope& sc, ast::conditional_expression_element* conditional) {
        if(!conditional)
            return;
        resolve_expression(sc, conditional->value);
        resolve_expression(sc, conditional->condition);
    }

    void resolve_selected_expression(scope& sc, ast::selected_expression_element* selected) {
        if(!selected)
            return;
        resolve_waveform_value(sc, selected->waveform);
        for(auto& choice : selected->choices)
            resolve_choice(sc, choice);
    }
};

} // namespace vhdl_fe
