#include "context_parser.h"
#include "ast/ast_nodes.h"
#include "hdlConvertor/vhdlConvertor/vhdlParser/vhdlParser.h"
#include "notImplementedLogger.h"
#include <algorithm>
#include <sstream>
namespace parser {
namespace context {
using namespace vhdl_antlr;
namespace {
std::string get_identifier(vhdlParser::IdentifierContext* ctx) {
    if(ctx->BASIC_IDENTIFIER())
        return ctx->BASIC_IDENTIFIER()->getText();
    if(ctx->EXTENDED_IDENTIFIER())
        return ctx->EXTENDED_IDENTIFIER()->getText();
    return std::string("work");
}
std::string get_literal(vhdlParser::Name_literalContext* ctx) {
    if(auto i = ctx->identifier())
        return get_identifier(i);
    if(auto o = ctx->operator_symbol())
        return o->STRING_LITERAL()->getText();
    if(auto c = ctx->CHARACTER_LITERAL())
        return c->getText();
    return "work";
}

std::string get_name(vhdlParser::NameContext* ctx) {
    if(ctx->name_literal())
        return get_literal(ctx->name_literal());
    if(ctx->external_name())
        NotImplementedLogger::print("external_name", ctx);
    if(ctx->name())
        NotImplementedLogger::print("name", ctx);
    return "";
}
} // namespace

ast::design_file* parse(vhdlParser::Design_fileContext* ctx, ast::ast_node_factory& anf) {
    // design_file
    // : ( design_unit )* EOF
    // ;
    auto* node = anf.create<ast::design_file>();
    for(auto u : ctx->design_unit()) {
        node->design_units.push_back(parse(u, anf));
    }
    return node;
}

ast::design_unit* parse(vhdlParser::Design_unitContext* ctx,
                        ast::ast_node_factory& anf) { // design_unit
    // : context_clause library_unit
    // ;
    auto* node = anf.create<ast::design_unit>();

    // context_clause: ( context_item )*;
    // context_item:
    //     library_clause
    //     | use_clause
    //     | context_reference
    // ;
    if(auto context = ctx->context_clause()) {
        for(auto ctx : context->context_item()) {
            if(auto n = ctx->library_clause())
                node->context_items.push_back(parse(n, anf));
            else if(auto n = ctx->use_clause())
                node->context_items.push_back(parse(n, anf));
            else if(auto n = ctx->context_reference())
                node->context_items.push_back(parse(n, anf));
        }
    }
    auto lib_unit = ctx->library_unit();
    // library_unit:
    //     primary_unit
    //     | secondary_unit
    // ;
    if(auto prim_unit = lib_unit->primary_unit()) {
        // primary_unit:
        //     entity_declaration
        //     | configuration_declaration
        //     | package_declaration
        //     | package_instantiation_declaration
        //     | context_declaration
        // ;
        if(auto n = prim_unit->entity_declaration()) {
            node->unit = parse(n, anf);
        } else if(auto n = prim_unit->configuration_declaration()) {
            node->unit = parse(n, anf);
        } else if(auto n = prim_unit->package_declaration()) {
            node->unit = parse(n, anf);
        } else if(auto n = prim_unit->package_instantiation_declaration()) {
            node->unit = parse(n, anf);
        } else if(auto n = prim_unit->context_declaration()) {
            node->unit = parse(n, anf);
        }
    } else if(auto sec_unit = lib_unit->secondary_unit()) {
        // secondary_unit:
        //     architecture_body
        //     | package_body
        // ;
        if(auto n = sec_unit->architecture_body()) {
            node->unit = parse(n, anf);
        } else if(auto n = sec_unit->package_body()) {
            node->unit = parse(n, anf);
        }
    }
    return node;
}

ast::library_clause* parse(vhdlParser::Library_clauseContext* ctx, ast::ast_node_factory& anf) {
    // library_clause: KW_LIBRARY logical_name_list SEMI;
    // logical_name_list: identifier_list;
    // identifier_list: identifier ( COMMA identifier )*;
    auto n = anf.create<ast::library_clause>();
    auto l = ctx->logical_name_list()->identifier_list()->identifier();
    std::transform(l.begin(), l.end(), std::back_insert_iterator(n->names), get_identifier);
    return nullptr;
}

ast::use_clause* parse(vhdlParser::Use_clauseContext* ctx, ast::ast_node_factory& anf) {
    // use_clause:
    //     KW_USE selected_name (COMMA selected_name)* SEMI
    // ;
    // selected_name:
    //       identifier (DOT suffix)*
    // ;
    // suffix:
    //     name_literal
    //     | KW_ALL
    // ;
    auto n = anf.create<ast::use_clause>();
    auto l = ctx->selected_name();
    std::transform(l.begin(), l.end(), std::back_insert_iterator(n->names), [](vhdlParser::Selected_nameContext* ctx) {
        std::stringstream oss;
        oss << ctx->identifier()->getText();
        for(auto suffix : ctx->suffix()) {
            if(suffix->KW_ALL())
                oss << ".all";
            else {
                oss << "." << get_literal(suffix->name_literal());
            }
        }
        return oss.str();
    });
    return n;
}
ast::context_reference* parse(vhdlParser::Context_referenceContext*, ast::ast_node_factory& anf) { return nullptr; }

ast::entity_declaration* parse(vhdlParser::Entity_declarationContext* ctx, ast::ast_node_factory& anf) {
    // entity_declaration:
    //       KW_ENTITY identifier KW_IS
    //           ( generic_clause )?
    //           ( port_clause )?
    //           ( entity_declarative_item )*
    //       ( KW_BEGIN ( entity_statement )* )?
    //       KW_END ( KW_ENTITY )? ( identifier )? SEMI
    // ;
    auto n = anf.create<ast::entity_declaration>();
    n->identifier = ctx->identifier(0)->getText();
    for(auto i : ctx->port_clause()->port_list()->interface_list()->interface_element()) {
        auto l = parse(i, anf);
        n->port_list.insert(n->port_list.end(), l.begin(), l.end());
    }
    return n;
}

std::vector<ast::interface_declaration*> parse(vhdl_antlr::vhdlParser::Interface_elementContext* ctx, ast::ast_node_factory& anf) {
    if(auto o = ctx->interface_declaration()->interface_object_declaration()) {
        if(auto c = o->interface_constant_declaration()) {
            NotImplementedLogger::print("interface_constant_declaration", ctx);
        }
        if(auto s = o->interface_signal_declaration()) {
            return parse(s, anf);
        }
        if(auto v = o->interface_variable_declaration()) {
            NotImplementedLogger::print("interface_variable_declaration", ctx);
        }
        if(auto f = o->interface_file_declaration()) {
            NotImplementedLogger::print("interface_file_declaration", ctx);
        }
        return std::vector<ast::interface_declaration*>();
    }
    if(ctx->interface_declaration()->interface_type_declaration()) {
        NotImplementedLogger::print("interface_type_declaration", ctx);
    }
    if(ctx->interface_declaration()->interface_subprogram_declaration()) {
        NotImplementedLogger::print("interface_subprogram_declaration", ctx);
    }
    if(ctx->interface_declaration()->interface_package_declaration()) {
        NotImplementedLogger::print("interface_package_declaration", ctx);
    }
    return std::vector<ast::interface_declaration*>();
}

std::vector<ast::interface_declaration*> parse(vhdlParser::Interface_signal_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::interface_declaration*> res;
    if(ctx->subtype_indication()->resolution_indication())
        NotImplementedLogger::print("resolution_indication", ctx);
    if(ctx->subtype_indication()->constraint())
        NotImplementedLogger::print("constraint", ctx);
    auto t = ctx->subtype_indication()->type_mark();
    for(auto i : ctx->identifier_list()->identifier()) {
        auto n = anf.create<ast::signal_declaration>();
        n->name = get_identifier(i);
        if(ctx->signal_mode()->KW_IN())
            n->mode = ast::signal_mode::IN;
        else if(ctx->signal_mode()->KW_OUT())
            n->mode = ast::signal_mode::OUT;
        else if(ctx->signal_mode()->KW_INOUT())
            n->mode = ast::signal_mode::INOUT;
        else if(ctx->signal_mode()->KW_BUFFER())
            n->mode = ast::signal_mode::BUFFER;
        else if(ctx->signal_mode()->KW_LINKAGE())
            n->mode = ast::signal_mode::LINKAGE;
        else
            n->mode = ast::signal_mode::NONE;
        n->type = get_name(t->name());
        n->is_bus = ctx->KW_BUS();
    }
    return res;
}

ast::configuration_declaration* parse(vhdlParser::Configuration_declarationContext*, ast::ast_node_factory& anf) { return nullptr; }

ast::package_declaration* parse(vhdlParser::Package_declarationContext*, ast::ast_node_factory& anf) { return nullptr; }

ast::package_instantiation_declaration* parse(vhdlParser::Package_instantiation_declarationContext*, ast::ast_node_factory& anf) {
    return nullptr;
}

ast::context_declaration* parse(vhdlParser::Context_declarationContext*, ast::ast_node_factory& anf) { return nullptr; }

ast::architecture_body* parse(vhdlParser::Architecture_bodyContext*, ast::ast_node_factory& anf) {
    // architecture_body:
    //       KW_ARCHITECTURE identifier KW_OF name KW_IS
    //           ( block_declarative_item )*
    //       KW_BEGIN
    //           ( concurrent_statement )*
    //       KW_END ( KW_ARCHITECTURE )? ( identifier )? SEMI
    // ;
    return nullptr;
}

ast::package_body* parse(vhdlParser::Package_bodyContext*, ast::ast_node_factory& anf) { return nullptr; }

} // namespace context
} // namespace parser