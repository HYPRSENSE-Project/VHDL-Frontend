#include "ast_printer.h"

#include <ostream>
#include <type_traits>

namespace {
std::string name_text(const ast::name_node* node) { return node ? node->text : std::string(); }

void print_string_list(std::ostream& os, const std::vector<std::string>& values) {
    for(size_t i = 0; i < values.size(); ++i) {
        if(i)
            os << ", ";
        os << values[i];
    }
}

void print_object(std::ostream& os, const ast::interface_declaration_item& item) {
    std::visit(
        [&os](auto* decl) {
            if(!decl)
                return;
            using T = std::decay_t<decltype(*decl)>;
            if constexpr(std::is_same_v<T, ast::signal_declaration>) {
                os << decl->name;
                if(!decl->type.empty())
                    os << " : " << decl->type;
            } else if constexpr(std::is_same_v<T, ast::interface_constant_declaration> ||
                                std::is_same_v<T, ast::interface_signal_declaration> ||
                                std::is_same_v<T, ast::interface_variable_declaration> ||
                                std::is_same_v<T, ast::interface_file_declaration>) {
                os << decl->identifier;
                if(decl->subtype_indic && !decl->subtype_indic->type->name->text.empty())
                    os << " : " << decl->subtype_indic->type;
            } else if constexpr(std::is_same_v<T, ast::constant_declaration> || std::is_same_v<T, ast::variable_declaration> ||
                                std::is_same_v<T, ast::file_declaration>) {
                print_string_list(os, decl->identifiers);
                if(decl->indication && !decl->indication->type.empty())
                    os << " : " << decl->indication->type;
            }
        },
        item);
}

void print_object_list(std::ostream& os, const char* title, const std::vector<ast::interface_declaration_item>& items) {
    os << "  " << title << ":\n";
    for(const auto& item : items) {
        os << "    ";
        print_object(os, item);
        os << '\n';
    }
}

void print_instantiation(std::ostream& os, const std::string& label, const ast::component_instantiation_statement* inst) {
    if(!inst)
        return;

    os << "    ";
    if(!label.empty())
        os << label << ": ";

    switch(inst->unit_kind) {
    case ast::instantiated_unit_kind_e::ENTITY:
        os << "entity ";
        break;
    case ast::instantiated_unit_kind_e::CONFIGURATION:
        os << "configuration ";
        break;
    case ast::instantiated_unit_kind_e::COMPONENT:
        os << "component ";
        break;
    }

    os << inst->unit_name;
    if(!inst->architecture_id.empty())
        os << '(' << inst->architecture_id << ')';
    os << '\n';
}

void print_statement_components(std::ostream& os, const ast::concurrent_statement* stmt);

void print_body_components(std::ostream& os, const ast::generate_statement_body& body) {
    for(const auto* stmt : body.concurrent_statements)
        print_statement_components(os, stmt);
}

void print_statement_components(std::ostream& os, const ast::concurrent_statement* stmt) {
    if(!stmt)
        return;

    std::visit(
        [&os, stmt](auto* node) {
            if(!node)
                return;
            using T = std::decay_t<decltype(*node)>;
            if constexpr(std::is_same_v<T, ast::component_instantiation_statement>) {
                print_instantiation(os, stmt->label.empty() ? node->label : stmt->label, node);
            } else if constexpr(std::is_same_v<T, ast::block_statement>) {
                for(const auto* child : node->concurrent_statements)
                    print_statement_components(os, child);
            } else if constexpr(std::is_same_v<T, ast::for_generate_statement>) {
                print_body_components(os, node->body);
            } else if constexpr(std::is_same_v<T, ast::if_generate_statement>) {
                for(const auto* clause : node->clauses) {
                    if(clause)
                        print_body_components(os, clause->body);
                }
                print_body_components(os, node->else_body);
            } else if constexpr(std::is_same_v<T, ast::case_generate_statement>) {
                for(const auto* alternative : node->alternatives) {
                    if(alternative)
                        print_body_components(os, alternative->body);
                }
            }
        },
        stmt->statement);
}

void print_selected_name(std::ostream& os, const ast::selected_name& name) {
    os << name.identifier;
    if(!name.suffix.empty())
        os << '.' << name.suffix;
}
} // namespace

void print_ast(std::ostream& os, ast::design_file* top) {
    if(!top)
        return;

    for(const auto& unit : top->units) {
        std::visit(
            [&os](auto* item) {
                if(!item)
                    return;
                using T = std::decay_t<decltype(*item)>;
                if constexpr(std::is_same_v<T, ast::library_clause>) {
                    os << "library ";
                    print_string_list(os, item->names);
                    os << '\n';
                } else if constexpr(std::is_same_v<T, ast::use_clause>) {
                    os << "use ";
                    for(auto pkg : item->clauses) {
                        os << pkg->identifier;
                        for(auto s : pkg->suffixes)
                            os << "." << s;
                        os << '\n';
                    }
                } else if constexpr(std::is_same_v<T, ast::context_reference>) {
                    os << "context ";
                    for(size_t i = 0; i < item->selected_names.size(); ++i) {
                        if(i)
                            os << ", ";
                        print_selected_name(os, item->selected_names[i]);
                    }
                    os << '\n';
                } else if constexpr(std::is_same_v<T, ast::entity_declaration>) {
                    os << "entity " << item->identifier << '\n';
                    print_object_list(os, "generics", item->generic_list);
                    print_object_list(os, "ports", item->port_list);
                } else if constexpr(std::is_same_v<T, ast::configuration_declaration>) {
                    os << "configuration " << item->identifier << '\n';
                } else if constexpr(std::is_same_v<T, ast::package_declaration>) {
                    os << "package " << item->identifier << '\n';
                } else if constexpr(std::is_same_v<T, ast::package_instantiation_declaration>) {
                    os << "package instantiation " << item->identifier << '\n';
                } else if constexpr(std::is_same_v<T, ast::context_declaration>) {
                    os << "context " << item->identifier << '\n';
                } else if constexpr(std::is_same_v<T, ast::architecture_body>) {
                    os << "architecture " << item->identifier << " of " << name_text(item->primary) << '\n';
                    os << "  component instantiations:\n";
                    for(const auto* stmt : item->concurrent_statements)
                        print_statement_components(os, stmt);
                } else if constexpr(std::is_same_v<T, ast::package_body>) {
                    os << "package body " << item->identifier << '\n';
                }
            },
            unit);
    }
}
