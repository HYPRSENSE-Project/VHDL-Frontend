// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#include "context_parser.h"
#include "vhdlParser/vhdlParser.h"
#include <algorithm>
#include <ast_node_factory.h>
#include <ast_nodes.h>
#include <sstream>
#include <stdexcept>

namespace parser {
namespace context {
using namespace vhdl_antlr;

namespace {
std::string get_identifier(vhdlParser::IdentifierContext* ctx) {
    if(ctx->BASIC_IDENTIFIER())
        return ctx->BASIC_IDENTIFIER()->getText();
    if(ctx->EXTENDED_IDENTIFIER())
        return ctx->EXTENDED_IDENTIFIER()->getText();
    throw std::runtime_error("Unsupported identifier");
}

std::string get_literal(vhdlParser::Name_literalContext* ctx) {
    if(auto i = ctx->identifier())
        return get_identifier(i);
    if(auto o = ctx->operator_symbol())
        return o->STRING_LITERAL()->getText();
    if(auto c = ctx->CHARACTER_LITERAL())
        return c->getText();
    throw std::runtime_error("Unsupported literal");
}

std::string get_optional_name(vhdlParser::NameContext* ctx) { return ctx->getText(); }

std::string parse_actual_designator(vhdlParser::Actual_designatorContext* ctx) { return ctx->getText(); }

std::string parse_actual_part(vhdlParser::Actual_partContext* ctx) {
    if(auto name = ctx->name()) {
        auto actual_designator = parse_actual_designator(ctx->actual_designator());
        if(actual_designator.empty())
            return get_optional_name(name);
        auto name_text = get_optional_name(name);
        std::string actual;
        actual.reserve(name_text.size() + actual_designator.size() + 2);
        actual += name_text;
        actual.push_back('(');
        actual += actual_designator;
        actual.push_back(')');
        return actual;
    }
    return parse_actual_designator(ctx->actual_designator());
}

std::string find_label_between(const std::vector<vhdlParser::LabelContext*>& labels, long long after_token, long long before_token) {
    for(auto label : labels) {
        auto start = label->getStart()->getTokenIndex();
        auto stop = label->getStop()->getTokenIndex();
        if(start > after_token && stop < before_token)
            return get_identifier(label->identifier());
    }
    return "";
}

std::string find_label_after(const std::vector<vhdlParser::LabelContext*>& labels, long long after_token) {
    for(auto label : labels) {
        if(label->getStart()->getTokenIndex() > after_token)
            return get_identifier(label->identifier());
    }
    return "";
}

std::string get_designator(vhdlParser::DesignatorContext* ctx) {
    if(!ctx)
        return "";
    if(auto id = ctx->identifier())
        return get_identifier(id);
    if(auto op = ctx->operator_symbol())
        return op->getText();
    return "";
}

bool is_valid_resolution_indication(vhdlParser::Resolution_indicationContext* ctx) {
    if(!ctx)
        return false;
    if(ctx->name())
        return true;
    auto element = ctx->element_resolution();
    if(!element)
        return false;
    if(auto array = element->array_element_resolution())
        return is_valid_resolution_indication(array->resolution_indication());
    if(auto record = element->record_resolution()) {
        for(auto entry : record->record_element_resolution()) {
            if(!is_valid_resolution_indication(entry->resolution_indication()))
                return false;
        }
        return true;
    }
    return false;
}

void get_text(antlr4::RuleContext* ctx, std::stringstream& ss) {
    if(ctx->children.empty()) {
        return;
    }

    for(size_t i = 0; i < ctx->children.size(); i++) {
        antlr4::tree::ParseTree* tree = ctx->children[i];
        if(auto rule_context = dynamic_cast<antlr4::RuleContext*>(tree))
            get_text(rule_context, ss);
        else {
            if(ss.str().size())
                ss << " ";
            ss << tree->getText();
        }
    }
}

std::string get_text(antlr4::ParserRuleContext* ctx) {
    if(ctx->children.empty()) {
        return "";
    }
    std::stringstream ss;
    get_text(ctx, ss);
    return ss.str();
}
} // namespace

ast::name_node* parse(vhdlParser::NameContext* ctx, ast::ast_node_factory& anf) {
    if(!ctx)
        return nullptr;

    auto node = anf.create<ast::name_node>();
    node->text = get_text(ctx);

    if(auto literal = ctx->name_literal()) {
        node->value = get_literal(literal);
        return node;
    }

    if(auto external = ctx->external_name()) {
        node->kind = ast::name_kind_e::EXTERNAL;
        node->value = get_text(external);
        return node;
    }

    node->prefix = parse(ctx->name(), anf);

    if(auto slice = ctx->name_slice_part()) {
        node->kind = ast::name_kind_e::SLICE;
        auto part = anf.create<ast::name_slice>();
        part->range = parse(slice->explicit_range(), anf);
        node->slice = part;
        return node;
    }

    if(auto attribute = ctx->name_attribute_part()) {
        node->kind = ast::name_kind_e::ATTRIBUTE;
        auto part = anf.create<ast::name_attribute>();
        if(auto signature = attribute->signature())
            part->signature = signature->getText();
        part->designator = attribute->attribute_designator()->getText();
        node->value = part->designator;
        node->attribute = part;
        return node;
    }

    if(auto associations = ctx->association_list()) {
        node->kind = ast::name_kind_e::CALL;
        auto part = anf.create<ast::name_arguments>();
        part->associations = parse(associations, anf);
        node->arguments = part;
        return node;
    }

    if(auto suffix = ctx->suffix()) {
        node->kind = ast::name_kind_e::SELECTED;
        node->value = suffix->KW_ALL() ? suffix->getText() : get_literal(suffix->name_literal());
    }

    return node;
}

ast::type_mark* parse(vhdlParser::Type_markContext* ctx, ast::ast_node_factory& anf) {
    if(!ctx)
        return nullptr;

    auto node = anf.create<ast::type_mark>();
    node->name = parse(ctx->name(), anf);
    return node;
}

ast::explicit_range* parse(vhdlParser::Explicit_rangeContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::explicit_range>();
    node->left = parse(ctx->simple_expression(0), anf);
    node->right = parse(ctx->simple_expression(1), anf);
    node->direction = ctx->direction()->KW_DOWNTO() ? ast::direction_e::DOWNTO : ast::direction_e::TO;
    return node;
}

ast::attribute_range* parse(vhdlParser::Attribute_nameContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::attribute_range>();
    node->name = ctx->name()->getText();
    node->attribute_designator = ctx->name_attribute_part()->attribute_designator()->getText();
    return node;
}

ast::subtype_indication* parse(vhdlParser::Subtype_indicationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subtype_indication>();
    if(auto resolution = ctx->resolution_indication())
        node->resolution = parse(resolution, anf);
    node->type = parse(ctx->type_mark(), anf);
    if(auto constraint = ctx->constraint())
        node->constr = parse(constraint, anf);
    return node;
}

ast::physical_unit_definition* parse(vhdlParser::Physical_type_definitionContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::physical_unit_definition>();
    node->primary_unit = get_identifier(ctx->primary_unit_declaration()->identifier());

    const auto& secondary_units = ctx->secondary_unit_declaration();
    for(auto secondary : secondary_units) {
        auto snode = anf.create<ast::secondary_unit>();
        auto literal = secondary->physical_literal();
        if(literal->DECIMAL_LITERAL())
            snode->physical_literal_type = ast::physical_unit_lieral_e::DECIMAL;
        else if(literal->BASED_LITERAL())
            snode->physical_literal_type = ast::physical_unit_lieral_e::BASED;
        snode->physical_literal_name = parse(literal->name(), anf);
        node->secondary_unit_identifiers.push_back(snode);
    }
    return node;
}

ast::type_definition_item parse(vhdlParser::Type_definitionContext* ctx, ast::ast_node_factory& anf) {
    if(auto scalar = ctx->scalar_type_definition()) {
        if(auto enumeration = scalar->enumeration_type_definition()) {
            auto node = anf.create<ast::enumeration_type_definition>();
            const auto& literals = enumeration->enumeration_literal();
            node->enumeration_literals.reserve(literals.size());
            for(auto literal : literals) {
                if(auto identifier = literal->identifier())
                    node->enumeration_literals.emplace_back(get_identifier(identifier));
                else if(auto character = literal->CHARACTER_LITERAL())
                    node->enumeration_literals.emplace_back(character->getText());
            }
            return node;
        }

        auto node = anf.create<ast::numeric_type_definition>();
        vhdlParser::RangeContext* range = nullptr;
        if(auto integer = scalar->integer_type_definition())
            range = integer->range_constraint()->range();
        else if(auto floating = scalar->floating_type_definition())
            range = floating->range_constraint()->range();
        else if(auto physical = scalar->physical_type_definition()) {
            range = physical->range_constraint()->range();
            node->physical_unit = parse(physical, anf);
        }
        if(range) {
            if(range->explicit_range())
                node->range = parse(range->explicit_range(), anf);
            else if(range->attribute_name())
                node->range = parse(range->attribute_name(), anf);
        }
        return node;
    }

    if(auto composite = ctx->composite_type_definition()) {
        if(auto atd = composite->array_type_definition()) {
            if(auto unbounded = atd->unbounded_array_definition()) {
                auto node = anf.create<ast::unbounded_array_definition>();
                const auto& definitions = unbounded->index_subtype_definition();
                node->index_subtype_definitions.reserve(definitions.size());
                for(auto definition : definitions)
                    node->index_subtype_definitions.emplace_back(parse(definition->type_mark(), anf));
                return node;
            } else if(auto constrained = atd->constrained_array_definition()) {
                auto node = anf.create<ast::constrained_array_definition>();
                const auto& ranges = constrained->index_constraint()->discrete_range();
                node->index_constraints.reserve(ranges.size());
                for(auto range : ranges)
                    node->index_constraints.emplace_back(parse(range, anf));
                node->subtype_indication_ = parse(constrained->subtype_indication(), anf);
                return node;
            }
        } else if(auto record = composite->record_type_definition()) {
            auto node = anf.create<ast::record_type_definition>();
            const auto& declarations = record->element_declaration();
            node->element_declarations.reserve(declarations.size());
            for(auto declaration : declarations) {
                for(auto identifier : declaration->identifier_list()->identifier()) {
                    auto element = anf.create<ast::element_declaration>();
                    element->identifier = get_identifier(identifier);
                    element->element_subtype_definition = parse(declaration->element_subtype_definition()->subtype_indication(), anf);
                    node->element_declarations.emplace_back(element);
                }
            }
            if(auto identifier = record->identifier())
                node->identifier = get_identifier(identifier);
            return node;
        }
    }

    if(auto access = ctx->access_type_definition())
        return parse(access->subtype_indication(), anf);

    if(auto file = ctx->file_type_definition()) {
        return parse(file->type_mark(), anf);
    }

    if(auto protected_type = ctx->protected_type_definition()) {
        auto node = anf.create<ast::protected_type_definition>();
        if(auto declaration = protected_type->protected_type_declaration()) {
            node->type = ast::protected_type_e::DECL;
            for(auto item : declaration->protected_type_declarative_item()) {
                if(auto subprogram = item->subprogram_declaration())
                    node->declarations.emplace_back(parse(subprogram, anf));
                else if(auto instantiation = item->subprogram_instantiation_declaration())
                    node->declarations.emplace_back(parse(instantiation, anf));
                else if(auto attribute = item->attribute_specification())
                    node->declarations.emplace_back(parse(attribute, anf));
                else if(auto use_clause = item->use_clause())
                    node->declarations.emplace_back(parse(use_clause, anf));
            }
        } else if(auto body = protected_type->protected_type_body()) {
            node->type = ast::protected_type_e::BODY;
            for(auto item : body->process_declarative_item()) {
                for(auto& parsed : parse(item, anf))
                    std::visit([node](auto value) { node->body_declarations.emplace_back(value); }, parsed);
            }
        }
        return node;
    }

    throw std::runtime_error("Unsupported type_definition");
}

ast::discrete_range_item parse(vhdlParser::Discrete_rangeContext* ctx, ast::ast_node_factory& anf) {
    if(auto subtype = ctx->subtype_indication())
        return parse(subtype, anf);

    auto range = ctx->range();
    if(auto attribute = range->attribute_name())
        return parse(attribute, anf);
    if(auto explicit_range = range->explicit_range())
        return parse(explicit_range, anf);

    throw std::runtime_error("Unsupported discrete_range");
}

ast::array_constraint* parse(vhdlParser::Array_constraintContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::array_constraint>();
    node->is_open = ctx->KW_OPEN();
    if(auto index = ctx->index_constraint()) {
        const auto& ranges = index->discrete_range();
        node->index_constraint.reserve(ranges.size());
        for(auto range : ranges)
            node->index_constraint.emplace_back(parse(range, anf));
    }
    return node;
}

ast::constraint_item parse(vhdlParser::Element_constraintContext* ctx, ast::ast_node_factory& anf) {
    if(auto array = ctx->array_constraint())
        return parse(array, anf);
    if(auto record = ctx->record_constraint())
        return parse(record, anf);

    throw std::runtime_error("Unsupported element_constraint");
}

ast::record_constraint* parse(vhdlParser::Record_constraintContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::record_constraint>();
    const auto& constraints = ctx->record_element_constraint();
    node->constraint.reserve(constraints.size());
    for(auto element : constraints) {
        ast::record_element_constraint item{};
        item.identifier = get_identifier(element->identifier());
        if(auto nested = element->element_constraint()) {
            auto indication = anf.create<ast::subtype_indication>();
            indication->constr = parse(nested, anf);
            item.indication = indication;
        }
        node->constraint.emplace_back(item);
    }
    return node;
}

ast::resolution_item parse(vhdlParser::Element_resolutionContext* ctx, ast::ast_node_factory& anf) {
    if(auto array = ctx->array_element_resolution())
        return parse(array->resolution_indication(), anf);

    if(auto record = ctx->record_resolution()) {
        auto node = anf.create<ast::record_resolution>();
        const auto& elements = record->record_element_resolution();
        node->elems.reserve(elements.size());
        for(auto element : elements) {
            auto item = anf.create<ast::record_element_resolution>();
            item->identifier = get_identifier(element->identifier());
            item->resolution = parse(element->resolution_indication(), anf);
            node->elems.emplace_back(item);
        }
        return node;
    }

    throw std::runtime_error("Unsupported element_resolution");
}

ast::resolution_indication* parse(vhdlParser::Resolution_indicationContext* ctx, ast::ast_node_factory& anf) {
    if(!is_valid_resolution_indication(ctx))
        throw std::runtime_error("Unsupported resolution_indication");

    auto node = anf.create<ast::resolution_indication>();
    if(auto name = ctx->name())
        node->name = name->getText();
    else
        node->elem_resolution = parse(ctx->element_resolution(), anf);
    return node;
}

ast::component_specification* parse(vhdlParser::Component_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::component_specification>();
    auto instantiation_list = ctx->instantiation_list();
    if(instantiation_list->KW_OTHERS()) {
        node->instantiations.emplace_back(instantiation_list->KW_OTHERS()->getText());
    } else if(instantiation_list->KW_ALL()) {
        node->instantiations.emplace_back(instantiation_list->KW_ALL()->getText());
    } else {
        const auto& labels = instantiation_list->label();
        node->instantiations.reserve(labels.size());
        for(auto label : labels)
            node->instantiations.emplace_back(get_identifier(label->identifier()));
    }
    node->name = ctx->name()->getText();
    return node;
}

ast::binding_indication* parse(vhdlParser::Binding_indicationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::binding_indication>();
    if(auto entity_aspect = ctx->entity_aspect()) {
        if(entity_aspect->KW_OPEN()) {
            node->type = ast::entity_aspect_e::OPEN;
        } else {
            node->unit_ref = entity_aspect->unit_ref()->getText();
            if(entity_aspect->KW_ENTITY()) {
                node->type = ast::entity_aspect_e::ENTITY;
                if(auto identifier = entity_aspect->identifier())
                    node->identifier = get_identifier(identifier);
            } else if(entity_aspect->KW_CONFIGURATION())
                node->type = ast::entity_aspect_e::CONFIGURATION;
        }
    }
    if(auto generic_map = ctx->generic_map_aspect())
        node->generic_map = parse(generic_map->association_list(), anf);
    if(auto port_map = ctx->port_map_aspect())
        node->port_map = parse(port_map->association_list(), anf);
    return node;
}

ast::configuration_specification* parse(vhdlParser::Configuration_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::configuration_specification>();
    if(auto simple = ctx->simple_configuration_specification()) {
        node->component_spec = parse(simple->component_specification(), anf);
        node->binding = parse(simple->binding_indication(), anf);
    } else if(auto compound = ctx->compound_configuration_specification()) {
        node->component_spec = parse(compound->component_specification(), anf);
        node->binding = parse(compound->binding_indication(), anf);
    }
    return node;
}

std::vector<ast::association_element*> parse(vhdlParser::Association_listContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::association_element*> associations;
    for(auto elem : ctx->association_element()) {
        auto assoc = anf.create<ast::association_element>();
        if(auto formal_part = elem->formal_part()) {
            assoc->formal_name = formal_part->name(0)->getText();
            if(formal_part->name().size() > 1)
                assoc->formal_paren_name = formal_part->name(1)->getText();
        }
        if(auto actual_part = elem->actual_part()) {
            if(auto n = actual_part->name())
                assoc->actual_name = n->getText();
            assoc->act_designator = anf.create<ast::actual_designator>();
            auto act_ctx = actual_part->actual_designator();
            assoc->act_designator->is_inertial = act_ctx->KW_INERTIAL();
            if(auto e = act_ctx->expression())
                assoc->act_designator->expr = parse(e, anf);
            if(auto i = act_ctx->subtype_indication())
                assoc->act_designator->indication = parse(i, anf);
            assoc->act_designator->is_open = act_ctx->KW_OPEN();
        }
        associations.push_back(assoc);
    }
    return associations;
}
ast::aggregate* parse(vhdl_antlr::vhdlParser::AggregateContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::aggregate>();
    return n;
}

bool is_character_literal_name(vhdlParser::NameContext* ctx) {
    return ctx && ctx->name_literal() && ctx->name_literal()->CHARACTER_LITERAL();
}

ast::primary_item parse(vhdlParser::Numeric_literalContext* ctx, ast::ast_node_factory& anf) {
    if(ctx->name() && !ctx->DECIMAL_LITERAL() && !ctx->BASED_LITERAL()) {
        if(!is_character_literal_name(ctx->name()))
            return parse(ctx->name(), anf);
    }

    auto n = anf.create<ast::literal_node>();
    n->text = get_text(ctx);
    return n;
}

ast::primary_item parse(vhdl_antlr::vhdlParser::PrimaryContext* ctx, ast::ast_node_factory& anf) {
    if(auto nl = ctx->numeric_literal()) {
        return parse(nl, anf);
    }
    if(auto nl = ctx->BIT_STRING_LITERAL()) {
        auto n = anf.create<ast::literal_node>();
        n->text = nl->getText();
        return n;
    }
    if(auto nl = ctx->KW_NULL()) {
        auto n = anf.create<ast::literal_node>();
        n->text = nl->getText();
        return n;
    }
    if(auto alloc = ctx->allocator()) {
        auto n = anf.create<ast::allocator>();
        if(auto stic = alloc->subtype_indication()) {
            auto sti = anf.create<ast::subtype_indication>();
            if(auto resolution = stic->resolution_indication())
                sti->resolution = parse(resolution, anf);
            sti->type = parse(stic->type_mark(), anf);
            if(auto constraint = stic->constraint())
                sti->constr = parse(constraint, anf);
            n->si = sti;
        }
        return n;
    }
    if(auto aggr = ctx->aggregate()) {
        return parse(aggr, anf);
    }
    if(auto qe = ctx->qualified_expression()) {
        auto n = anf.create<ast::qualified_expression>();
        n->type = parse(qe->type_mark(), anf);
        n->aggr = parse(qe->aggregate(), anf);
        return n;
    }
    return ast::primary_item();
}

ast::simple_expression* parse(vhdlParser::Simple_expressionContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::simple_expression>();
    const auto primaries = ctx->primary();
    if(!primaries.empty()) {
        node->first_primary = parse(primaries.front(), anf);
        if(ctx->DOUBLESTAR() && primaries.size() > 1) {
            node->kind = ast::simple_expression_kind_e::POWER;
            node->op = ast::operation_kind_e::DOUBLESTAR;
            node->secondary_primary = parse(primaries[1], anf);
        } else {
            node->kind = ast::simple_expression_kind_e::PRIMARY;
        }
    }
    if(ctx->KW_ABS() || ctx->KW_NOT() || ctx->logical_operator() || ctx->sign()) {
        node->kind = ast::simple_expression_kind_e::PREFIX;
        if(ctx->KW_ABS())
            node->op = ast::operation_kind_e::ABS;
        else if(ctx->KW_NOT())
            node->op = ast::operation_kind_e::NOT;
        else if(ctx->logical_operator()) {
            auto log_op = ctx->logical_operator();
            if(log_op->KW_AND())
                node->op = ast::operation_kind_e::AND;
            else if(log_op->KW_OR())
                node->op = ast::operation_kind_e::OR;
            else if(log_op->KW_NAND())
                node->op = ast::operation_kind_e::NAND;
            else if(log_op->KW_NOR())
                node->op = ast::operation_kind_e::NOR;
            else if(log_op->KW_XOR())
                node->op = ast::operation_kind_e::XOR;
            else if(log_op->KW_XNOR())
                node->op = ast::operation_kind_e::XNOR;
        } else {
            auto sign = ctx->sign();
            if(sign->PLUS())
                node->op = ast::operation_kind_e::SIGN_PLUS;
            else if(sign->MINUS())
                node->op = ast::operation_kind_e::SIGN_MINUS;
        }
        node->operand = parse(ctx->simple_expression(0), anf);
    } else if(ctx->multiplying_operator()) {
        node->kind = ast::simple_expression_kind_e::MUL_DIV;
        auto oper = ctx->multiplying_operator();
        if(oper->MUL())
            node->op = ast::operation_kind_e::MUL;
        if(oper->DIV())
            node->op = ast::operation_kind_e::DIV;
        if(oper->KW_MOD())
            node->op = ast::operation_kind_e::MOD;
        if(oper->KW_REM())
            node->op = ast::operation_kind_e::REM;
        node->lhs = parse(ctx->simple_expression(0), anf);
        node->rhs = parse(ctx->simple_expression(1), anf);
    } else if(ctx->adding_operator()) {
        node->kind = ast::simple_expression_kind_e::ADD_SUB;
        auto oper = ctx->adding_operator();
        if(oper->PLUS())
            node->op = ast::operation_kind_e::PLUS;
        else if(oper->MINUS())
            node->op = ast::operation_kind_e::MINUS;
        else if(oper->AMPERSAND())
            node->op = ast::operation_kind_e::AMPERSAND;
        node->lhs = parse(ctx->simple_expression(0), anf);
        node->rhs = parse(ctx->simple_expression(1), anf);
    }
    return node;
}

ast::expression_item parse(vhdlParser::ExpressionContext* ctx, ast::ast_node_factory& anf) {
    if(ctx->COND_OP()) {
        auto node = anf.create<ast::conditional_primary>();
        node->prim = parse(ctx->primary(), anf);
        return node;
    }

    if(auto simple = ctx->simple_expression()) {
        return parse(simple, anf);
    }

    auto node = anf.create<ast::binary_expression>();
    if(ctx->shift_operator()) {
        auto oper = ctx->shift_operator();
        if(oper->KW_SLL())
            node->op = ast::operation_kind_e::SLL;
        else if(oper->KW_SRL())
            node->op = ast::operation_kind_e::SRL;
        else if(oper->KW_SLA())
            node->op = ast::operation_kind_e::SLA;
        else if(oper->KW_SRA())
            node->op = ast::operation_kind_e::SRA;
        else if(oper->KW_ROL())
            node->op = ast::operation_kind_e::ROL;
        else if(oper->KW_ROR())
            node->op = ast::operation_kind_e::ROR;
    } else if(ctx->relational_operator()) {
        auto oper = ctx->relational_operator();
        if(oper->EQ())
            node->op = ast::operation_kind_e::EQ;
        if(oper->NE())
            node->op = ast::operation_kind_e::NE;
        if(oper->LT())
            node->op = ast::operation_kind_e::LT;
        if(oper->CONASGN())
            node->op = ast::operation_kind_e::CONASGN;
        if(oper->GT())
            node->op = ast::operation_kind_e::GT;
        if(oper->GE())
            node->op = ast::operation_kind_e::GE;
        if(oper->EQ_MATCH())
            node->op = ast::operation_kind_e::EQ_MATCH;
        if(oper->NE_MATCH())
            node->op = ast::operation_kind_e::NE_MATCH;
        if(oper->LT_MATCH())
            node->op = ast::operation_kind_e::LT_MATCH;
        if(oper->LE_MATCH())
            node->op = ast::operation_kind_e::LE_MATCH;
        if(oper->GT_MATCH())
            node->op = ast::operation_kind_e::GT_MATCH;
        if(oper->GE_MATCH())
            node->op = ast::operation_kind_e::GE_MATCH;
    } else if(ctx->logical_operator()) {
        auto oper = ctx->logical_operator();
        if(oper->KW_AND())
            node->op = ast::operation_kind_e::AND;
        if(oper->KW_OR())
            node->op = ast::operation_kind_e::OR;
        if(oper->KW_NAND())
            node->op = ast::operation_kind_e::NAND;
        if(oper->KW_NOR())
            node->op = ast::operation_kind_e::NOR;
        if(oper->KW_XOR())
            node->op = ast::operation_kind_e::XOR;
        if(oper->KW_XNOR())
            node->op = ast::operation_kind_e::XNOR;
    }
    if(ctx->expression().size() > 0)
        node->lhs = parse(ctx->expression(0), anf);
    if(ctx->expression().size() > 1)
        node->rhs = parse(ctx->expression(1), anf);
    return node;
}

ast::expression_item parse(vhdlParser::ConditionContext* ctx, ast::ast_node_factory& anf) { return parse(ctx->expression(), anf); }

std::vector<ast::interface_constant_declaration*> parse(vhdlParser::Interface_constant_declarationContext* ctx,
                                                        ast::ast_node_factory& anf) {
    std::vector<ast::interface_constant_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::interface_constant_declaration>();
        node->identifier = get_identifier(id);
        node->is_in = ctx->KW_IN() != nullptr;
        node->subtype_indic = parse(ctx->subtype_indication(), anf);
        if(auto expr = ctx->expression())
            node->expression = parse(expr, anf);
        res.push_back(node);
    }
    return res;
}

std::vector<ast::interface_signal_declaration*> parse(vhdlParser::Interface_signal_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::interface_signal_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::interface_signal_declaration>();
        node->identifier = get_identifier(id);
        if(auto mode = ctx->signal_mode()) {
            if(mode->KW_IN())
                node->signal_mode = ast::signal_mode_e::IN;
            else if(mode->KW_OUT())
                node->signal_mode = ast::signal_mode_e::OUT;
            else if(mode->KW_INOUT())
                node->signal_mode = ast::signal_mode_e::INOUT;
            else if(mode->KW_BUFFER())
                node->signal_mode = ast::signal_mode_e::BUFFER;
            else if(mode->KW_LINKAGE())
                node->signal_mode = ast::signal_mode_e::LINKAGE;
        }
        node->subtype_indic = parse(ctx->subtype_indication(), anf);
        node->is_bus = ctx->KW_BUS() != nullptr;
        if(auto expr = ctx->expression())
            node->expression = parse(expr, anf);
        res.push_back(node);
    }
    return res;
}

std::vector<ast::interface_variable_declaration*> parse(vhdlParser::Interface_variable_declarationContext* ctx,
                                                        ast::ast_node_factory& anf) {
    std::vector<ast::interface_variable_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::interface_variable_declaration>();
        node->identifier = get_identifier(id);
        if(auto mode = ctx->signal_mode()) {
            if(mode->KW_IN())
                node->signal_mode = ast::signal_mode_e::IN;
            else if(mode->KW_OUT())
                node->signal_mode = ast::signal_mode_e::OUT;
            else if(mode->KW_INOUT())
                node->signal_mode = ast::signal_mode_e::INOUT;
            else if(mode->KW_BUFFER())
                node->signal_mode = ast::signal_mode_e::BUFFER;
            else if(mode->KW_LINKAGE())
                node->signal_mode = ast::signal_mode_e::LINKAGE;
        }
        node->subtype_indic = parse(ctx->subtype_indication(), anf);
        if(auto expr = ctx->expression())
            node->expression = parse(expr, anf);
        res.push_back(node);
    }
    return res;
}

std::vector<ast::interface_file_declaration*> parse(vhdlParser::Interface_file_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::interface_file_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::interface_file_declaration>();
        node->identifier = get_identifier(id);
        node->subtype_indic = parse(ctx->subtype_indication(), anf);
        res.push_back(node);
    }
    return res;
}

ast::interface_type_declaration* parse(vhdlParser::Interface_type_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::interface_type_declaration>();
    node->identifier = get_identifier(ctx->interface_incomplete_type_declaration()->identifier());
    return node;
}

static void append_interface_declaration_items(std::vector<ast::interface_declaration_item>& target, vhdlParser::Interface_listContext* ctx,
                                               ast::ast_node_factory& anf) {
    if(!ctx)
        return;
    const auto& elems = ctx->interface_element();
    target.reserve(target.size() + elems.size());
    for(auto elem : elems) {
        auto items = parse(elem, anf);
        target.insert(target.end(), items.begin(), items.end());
    }
}

ast::interface_procedure_specification* parse(vhdlParser::Interface_procedure_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::interface_procedure_specification>();
    node->designator = get_designator(ctx->designator());
    node->is_parameter = ctx->KW_PARAMETER() != nullptr;
    if(auto params = ctx->formal_parameter_list())
        append_interface_declaration_items(node->formal_parameter_list, params->interface_list(), anf);
    return node;
}

ast::interface_function_specification* parse(vhdlParser::Interface_function_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::interface_function_specification>();
    node->is_pure = ctx->KW_PURE() != nullptr;
    node->is_impure = ctx->KW_IMPURE() != nullptr;
    node->designator = get_designator(ctx->designator());
    if(auto params = ctx->formal_parameter_list())
        append_interface_declaration_items(node->formal_parameter_list, params->interface_list(), anf);
    node->return_type_mark = parse(ctx->type_mark(), anf);
    return node;
}

ast::interface_subprogram_declaration* parse(vhdlParser::Interface_subprogram_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::interface_subprogram_declaration>();
    auto spec = ctx->interface_subprogram_specification();
    if(auto procedure = spec->interface_procedure_specification())
        node->nterface_subprogram_specification = parse(procedure, anf);
    else if(auto function = spec->interface_function_specification())
        node->nterface_subprogram_specification = parse(function, anf);
    if(auto default_spec = ctx->interface_subprogram_default()) {
        node->is_box = default_spec->BOX() != nullptr;
        if(auto name = default_spec->name())
            node->interface_subprogram_default = parse(name, anf);
    }
    return node;
}

ast::interface_package_declaration* parse(vhdlParser::Interface_package_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::interface_package_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    node->name = parse(ctx->name(), anf);
    auto map = ctx->interface_package_generic_map_aspect();
    if(auto generic_map = map->generic_map_aspect())
        node->generic_map_aspect = parse(generic_map->association_list(), anf);
    else if(map->BOX())
        node->map_type = ast::generic_map_aspect_e::BOX;
    else if(map->KW_DEFAULT())
        node->map_type = ast::generic_map_aspect_e::DEFAULT;
    return node;
}

std::vector<ast::interface_declaration_item> parse(vhdlParser::Interface_elementContext* i, ast::ast_node_factory& anf) {
    // interface_element: interface_declaration;
    // interface_declaration:
    //       interface_object_declaration
    //       | interface_type_declaration
    //       | interface_subprogram_declaration
    //       | interface_package_declaration
    // ;
    // interface_object_declaration:
    //       interface_constant_declaration
    //       | interface_signal_declaration
    //       | interface_variable_declaration
    //       | interface_file_declaration
    // ;
    // interface_type_declaration:
    //       interface_incomplete_type_declaration
    // ;
    // interface_incomplete_type_declaration: KW_TYPE identifier;
    // interface_subprogram_declaration:
    //       interface_subprogram_specification ( KW_IS interface_subprogram_default )?
    // ;
    // interface_package_declaration:
    //       KW_PACKAGE identifier KW_IS KW_NEW name interface_package_generic_map_aspect
    // ;
    // interface_constant_declaration:
    //       KW_CONSTANT identifier_list COLON ( KW_IN )? subtype_indication ( VARASGN expression )?
    // ;
    //  interface_signal_declaration:
    //       ( KW_SIGNAL )? identifier_list COLON ( signal_mode )? subtype_indication ( KW_BUS )? ( VARASGN expression )?
    // ;
    // interface_variable_declaration:
    //       KW_VARIABLE identifier_list COLON ( signal_mode )? subtype_indication ( VARASGN expression )?
    // ;
    // interface_file_declaration:
    //       KW_FILE identifier_list COLON subtype_indication
    // ;
    if(auto o = i->interface_declaration()->interface_object_declaration()) {
        if(auto c = o->interface_constant_declaration()) {
            std::vector<ast::interface_declaration_item> res;
            auto items = parse(c, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        } else if(auto s = o->interface_signal_declaration()) {
            std::vector<ast::interface_declaration_item> res;
            auto items = parse(s, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        } else if(auto v = o->interface_variable_declaration()) {
            std::vector<ast::interface_declaration_item> res;
            auto items = parse(v, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        } else if(auto f = o->interface_file_declaration()) {
            std::vector<ast::interface_declaration_item> res;
            auto items = parse(f, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
    } else if(auto t = i->interface_declaration()->interface_type_declaration()) {
        return {parse(t, anf)};
    } else if(auto d = i->interface_declaration()->interface_subprogram_declaration()) {
        return {parse(d, anf)};
    } else if(auto p = i->interface_declaration()->interface_package_declaration()) {
        return {parse(p, anf)};
    }
    throw std::runtime_error("Unsupported interface_element");
}

std::vector<ast::disconnection_specification*> parse(vhdlParser::Disconnection_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto spec = ctx->guarded_signal_specification();
    auto signal_list = spec->signal_list();
    if(signal_list->KW_OTHERS()) {
        auto node = anf.create<ast::disconnection_specification>();
        node->type = parse(spec->type_mark(), anf);
        node->after_expression = parse(ctx->expression(), anf);
        node->signal_name = signal_list->KW_OTHERS()->getText();
        return {node};
    } else if(signal_list->KW_ALL()) {
        auto node = anf.create<ast::disconnection_specification>();
        node->type = parse(spec->type_mark(), anf);
        node->after_expression = parse(ctx->expression(), anf);
        node->signal_name = signal_list->KW_ALL()->getText();
        return {node};
    } else {
        std::vector<ast::disconnection_specification*> res;
        for(auto name : signal_list->name()) {
            auto node = anf.create<ast::disconnection_specification>();
            node->type = parse(spec->type_mark(), anf);
            node->after_expression = parse(ctx->expression(), anf);
            node->signal_name = name->getText();
        }
        return res;
    }
    throw std::runtime_error("Unsupported signal_list element");
}

std::vector<ast::entity_declarative_item> parse(vhdlParser::Entity_declarative_itemContext* e, ast::ast_node_factory& anf) {
    std::vector<ast::entity_declarative_item> res;
    if(auto sig = e->signal_declaration()) {
        auto elems = parse(sig, anf);
        res.insert(res.end(), elems.begin(), elems.end());
    } else if(auto p = e->process_declarative_item()) {
        for(auto item : parse(p, anf))
            std::visit([&res](auto value) { res.emplace_back(value); }, item);
    } else if(auto d = e->disconnection_specification()) {
        for(auto item : parse(d, anf))
            res.emplace_back(item);
    }
    return res;
}

std::vector<ast::block_declarative_item> parse(vhdlParser::Block_declarative_itemContext* b, ast::ast_node_factory& anf) {
    std::vector<ast::block_declarative_item> res;
    if(auto e = b->entity_declarative_item()) {
        auto decl = parse(e, anf);
        res.reserve(res.size() + decl.size());
        for(const auto& elem : decl) {
            std::visit([&res](const auto& value) { res.emplace_back(value); }, elem);
        }
    } else if(auto c = b->component_declaration()) {
        res.emplace_back(parse(c, anf));
    } else if(auto c = b->configuration_specification()) {
        res.emplace_back(parse(c, anf));
    }
    return res;
}

void parse(vhdlParser::Generate_statement_bodyContext* ctx, ast::generate_statement_body& body, ast::ast_node_factory& anf) {
    for(auto b : ctx->block_declarative_item()) {
        auto items = parse(b, anf);
        body.block_declarative_items.insert(body.block_declarative_items.end(), items.begin(), items.end());
    }
    for(auto c : ctx->concurrent_statement()) {
        body.concurrent_statements.emplace_back(parse(c, anf));
    }
}

void parse(vhdlParser::Generate_statement_body_with_begin_endContext* ctx, ast::generate_statement_body& body, ast::ast_node_factory& anf) {
    for(auto b : ctx->block_declarative_item()) {
        auto items = parse(b, anf);
        body.block_declarative_items.insert(body.block_declarative_items.end(), items.begin(), items.end());
    }
    for(auto c : ctx->concurrent_statement()) {
        body.concurrent_statements.emplace_back(parse(c, anf));
    }
}

std::vector<ast::choice_item> parse(vhdlParser::ChoicesContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::choice_item> res;
    for(auto choice_ctx : ctx->choice()) {
        if(auto simple_expression = choice_ctx->simple_expression())
            res.push_back(parse(simple_expression, anf));
        else if(auto discrete_range = choice_ctx->discrete_range()) {
            auto parsed_range = parse(discrete_range, anf);
            std::visit([&res](auto item) { res.emplace_back(item); }, parsed_range);
        } else if(choice_ctx->KW_OTHERS()) {
            auto n = anf.create<ast::literal_node>();
            n->text = "OTHERS";
            res.push_back(n);
        }
    }
    return res;
}

ast::design_file* parse(vhdlParser::Design_fileContext* ctx, ast::ast_node_factory& anf) {
    // design_file
    // : ( design_unit )* EOF
    // ;
    auto node = anf.create<ast::design_file>();
    for(auto u : ctx->design_unit()) {
        // : context_clause library_unit
        // ;
        // context_clause: ( context_item )*;
        // context_item:
        //     library_clause
        //     | use_clause
        //     | context_reference
        // ;
        if(auto context = u->context_clause()) {
            for(auto ctx : context->context_item()) {
                if(auto n = ctx->library_clause())
                    node->units.emplace_back(parse(n, anf));
                else if(auto n = ctx->use_clause())
                    node->units.emplace_back(parse(n, anf));
                else if(auto n = ctx->context_reference())
                    node->units.emplace_back(parse(n, anf));
            }
        }
        auto lib_unit = u->library_unit();
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
                node->units.emplace_back(parse(n, anf));
            } else if(auto n = prim_unit->configuration_declaration()) {
                node->units.emplace_back(parse(n, anf));
            } else if(auto n = prim_unit->package_declaration()) {
                node->units.emplace_back(parse(n, anf));
            } else if(auto n = prim_unit->package_instantiation_declaration()) {
                node->units.emplace_back(parse(n, anf));
            } else if(auto n = prim_unit->context_declaration()) {
                node->units.emplace_back(parse(n, anf));
            }
        } else if(auto sec_unit = lib_unit->secondary_unit()) {
            // secondary_unit:
            //     architecture_body
            //     | package_body
            // ;
            if(auto n = sec_unit->architecture_body()) {
                node->units.emplace_back(parse(n, anf));
            } else if(auto n = sec_unit->package_body()) {
                node->units.emplace_back(parse(n, anf));
            }
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
    return n;
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
    std::transform(l.begin(), l.end(), std::back_insert_iterator(n->clauses), [&anf](vhdlParser::Selected_nameContext* ctx) {
        auto pkg = anf.create<ast::used_package>();
        pkg->identifier = get_identifier(ctx->identifier());
        for(auto s : ctx->suffix())
            if(s->KW_ALL())
                pkg->suffixes.push_back("all");
            else
                pkg->suffixes.push_back(get_literal(s->name_literal()));
        return pkg;
    });
    return n;
}

ast::component_declaration* parse(vhdlParser::Component_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::component_declaration>();
    node->identifier = get_identifier(ctx->identifier(0));
    if(auto generic_clause = ctx->generic_clause()) {
        for(auto elem : generic_clause->generic_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
        }
    }
    if(auto port_clause = ctx->port_clause()) {
        for(auto elem : port_clause->port_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->port_list.insert(node->port_list.end(), items.begin(), items.end());
        }
    }
    return node;
}

ast::subprogram_declaration* parse(vhdlParser::Subprogram_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subprogram_declaration>();
    if(auto procedure = ctx->procedure_specification()) {
        node->is_function = false;
        node->designator = get_designator(procedure->designator());
        if(auto header = procedure->subprogram_header()) {
            for(auto elem : header->generic_list()->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
            }
            if(auto generic_map = header->generic_map_aspect())
                node->generic_map = parse(generic_map->association_list(), anf);
        }
        if(auto params = procedure->formal_parameter_list()) {
            for(auto elem : params->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->parameter_list.insert(node->parameter_list.end(), items.begin(), items.end());
            }
        }
    } else if(auto function = ctx->function_specification()) {
        node->is_function = true;
        node->designator = get_designator(function->designator());
        node->return_type = parse(function->type_mark(), anf);
        if(auto header = function->subprogram_header()) {
            for(auto elem : header->generic_list()->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
            }
            if(auto generic_map = header->generic_map_aspect())
                node->generic_map = parse(generic_map->association_list(), anf);
        }
        if(auto params = function->formal_parameter_list()) {
            for(auto elem : params->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->parameter_list.insert(node->parameter_list.end(), items.begin(), items.end());
            }
        }
    }
    return node;
}

ast::subprogram_declaration* parse(vhdlParser::Subprogram_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subprogram_declaration>();
    auto subprogram_ctx = ctx->subprogram_specification();
    if(auto procedure = subprogram_ctx->procedure_specification()) {
        node->is_function = false;
        node->designator = get_designator(procedure->designator());
        if(auto header = procedure->subprogram_header()) {
            for(auto elem : header->generic_list()->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
            }
            if(auto generic_map = header->generic_map_aspect())
                node->generic_map = parse(generic_map->association_list(), anf);
        }
        if(auto params = procedure->formal_parameter_list()) {
            for(auto elem : params->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->parameter_list.insert(node->parameter_list.end(), items.begin(), items.end());
            }
        }
    }
    if(auto function = subprogram_ctx->function_specification()) {
        node->is_function = true;
        node->designator = get_designator(function->designator());
        node->return_type = parse(function->type_mark(), anf);
        if(auto header = function->subprogram_header()) {
            for(auto elem : header->generic_list()->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
            }
            if(auto generic_map = header->generic_map_aspect())
                node->generic_map = parse(generic_map->association_list(), anf);
        }
        if(auto params = function->formal_parameter_list()) {
            for(auto elem : params->interface_list()->interface_element()) {
                auto items = parse(elem, anf);
                node->parameter_list.insert(node->parameter_list.end(), items.begin(), items.end());
            }
        }
    }
    return node;
}

ast::subprogram_instantiation_declaration* parse(vhdlParser::Subprogram_instantiation_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subprogram_instantiation_declaration>();
    node->designator = get_designator(ctx->designator());
    node->target_name = parse(ctx->name(), anf);
    return node;
}

ast::type_declaration* parse(vhdlParser::Type_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::type_declaration>();
    if(auto full = ctx->full_type_declaration()) {
        node->identifier = get_identifier(full->identifier());
        node->type = parse(full->type_definition(), anf);
    } else if(auto incomplete = ctx->incomplete_type_declaration()) {
        node->identifier = get_identifier(incomplete->identifier());
    }
    return node;
}

ast::subtype_declaration* parse(vhdlParser::Subtype_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subtype_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    node->indication = parse(ctx->subtype_indication(), anf);
    return node;
}

std::vector<ast::constant_declaration*> parse(vhdlParser::Constant_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::constant_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::constant_declaration>();
        node->identifier = get_identifier(id);
        node->indication = parse(ctx->subtype_indication(), anf);
        if(auto expr = ctx->expression())
            node->expr = parse(expr, anf);
        res.push_back(node);
    }
    return res;
}

std::vector<ast::variable_declaration*> parse(vhdlParser::Variable_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::variable_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::variable_declaration>();
        node->shared = ctx->KW_SHARED() != nullptr;
        node->identifier = get_identifier(id);
        node->indication = parse(ctx->subtype_indication(), anf);
        if(auto expr = ctx->expression())
            node->expr = parse(expr, anf);
        res.push_back(node);
    }
    return res;
}

std::vector<ast::file_declaration*> parse(vhdlParser::File_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::file_declaration*> res;
    for(auto id : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::file_declaration>();
        node->identifier = get_identifier(id);
        node->indication = parse(ctx->subtype_indication(), anf);
        if(auto open_info = ctx->file_open_information()) {
            if(open_info->KW_OUT())
                node->dir = ast::file_open_mode_e::OUT;
            if(open_info->KW_IN())
                node->dir = ast::file_open_mode_e::IN;
            if(open_info->KW_OPEN())
                node->open_expression = parse(open_info->expression(), anf);
            if(auto logical_name = open_info->file_logical_name())
                node->file_logical_name = parse(logical_name->expression(), anf);
        }
        res.push_back(node);
    }
    return res;
}

ast::alias_declaration* parse(vhdlParser::Alias_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::alias_declaration>();
    node->alias_designator = ctx->alias_designator()->getText();
    if(auto indic = ctx->subtype_indication())
        node->indication = parse(indic, anf);
    node->name = ctx->name()->getText();
    if(auto signature_ctx = ctx->signature()) {
        const auto& type_marks = signature_ctx->type_mark();
        const auto type_count = type_marks.size();
        const auto has_return = signature_ctx->KW_RETURN() != nullptr;
        const auto parameter_count = has_return && type_count > 0 ? type_count - 1 : type_count;
        node->type_marks.reserve(parameter_count);
        for(size_t i = 0; i < parameter_count; ++i)
            node->type_marks.emplace_back(parse(type_marks[i], anf));
        if(has_return && type_count > 0)
            node->return_type_mark = parse(type_marks.back(), anf);
    }
    return node;
}

ast::attribute_declaration* parse(vhdlParser::Attribute_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::attribute_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    node->type = parse(ctx->type_mark(), anf);
    return node;
}

ast::entity_class_e parse(vhdlParser::Entity_classContext* ctx) {
    if(ctx->KW_ENTITY())
        return ast::entity_class_e::ENTITY;
    if(ctx->KW_ARCHITECTURE())
        return ast::entity_class_e::ARCHITECTURE;
    if(ctx->KW_CONFIGURATION())
        return ast::entity_class_e::CONFIGURATION;
    if(ctx->KW_PROCEDURE())
        return ast::entity_class_e::PROCEDURE;
    if(ctx->KW_FUNCTION())
        return ast::entity_class_e::FUNCTION;
    if(ctx->KW_PACKAGE())
        return ast::entity_class_e::PACKAGE;
    if(ctx->KW_TYPE())
        return ast::entity_class_e::TYPE;
    if(ctx->KW_SUBTYPE())
        return ast::entity_class_e::SUBTYPE;
    if(ctx->KW_CONSTANT())
        return ast::entity_class_e::CONSTANT;
    if(ctx->KW_SIGNAL())
        return ast::entity_class_e::SIGNAL;
    if(ctx->KW_VARIABLE())
        return ast::entity_class_e::VARIABLE;
    if(ctx->KW_COMPONENT())
        return ast::entity_class_e::COMPONENT;
    if(ctx->KW_LABEL())
        return ast::entity_class_e::LABEL;
    if(ctx->KW_LITERAL())
        return ast::entity_class_e::LITERAL;
    if(ctx->KW_UNITS())
        return ast::entity_class_e::UNITS;
    if(ctx->KW_GROUP())
        return ast::entity_class_e::GROUP;
    if(ctx->KW_FILE())
        return ast::entity_class_e::FILE;
    if(ctx->KW_PROPERTY())
        return ast::entity_class_e::PROPERTY;
    return ast::entity_class_e::SEQUENCE;
}

ast::attribute_specification* parse(vhdlParser::Attribute_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::attribute_specification>();
    node->attribute_designator = ctx->attribute_designator()->getText();
    if(auto spec = ctx->entity_specification()) {
        if(auto names = spec->entity_name_list()) {
            if(names->KW_OTHERS()) {
                node->entity_name_list.emplace_back(names->KW_OTHERS()->getText());
            } else if(names->KW_ALL()) {
                node->entity_name_list.emplace_back(names->KW_ALL()->getText());
            } else {
                auto& entity_names = node->entity_name_list;
                entity_names.reserve(names->entity_designator().size());
                for(auto name : names->entity_designator())
                    entity_names.emplace_back(name->getText());
            }
        }
        node->entity_class = parse(spec->entity_class());
    }
    node->expr = parse(ctx->expression(), anf);
    return node;
}

ast::group_template_declaration* parse(vhdlParser::Group_template_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::group_template_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    if(auto entries = ctx->entity_class_entry_list()) {
        node->entity_class_entry_list.reserve(entries->entity_class_entry().size());
        for(auto entry : entries->entity_class_entry()) {
            auto ast_entry = anf.create<ast::entity_class_entry>();
            ast_entry->entity_class = parse(entry->entity_class());
            ast_entry->is_box = entry->BOX() != nullptr;
            node->entity_class_entry_list.emplace_back(ast_entry);
        }
    }
    return node;
}

ast::group_declaration* parse(vhdlParser::Group_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::group_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    node->name = get_optional_name(ctx->name());
    if(auto constituents = ctx->group_constituent_list()) {
        node->group_constituent_list.reserve(constituents->group_constituent().size());
        for(auto constituent : constituents->group_constituent())
            node->group_constituent_list.emplace_back(get_optional_name(constituent->name()));
    }
    return node;
}

std::vector<ast::package_body_declarative_item> parse(vhdlParser::Process_or_package_declarative_itemContext* ctx,
                                                      ast::ast_node_factory& anf) {
    std::vector<ast::package_body_declarative_item> res;
    if(auto item = ctx->subprogram_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->subprogram_instantiation_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->package_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->package_instantiation_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->type_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->subtype_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->constant_declaration()) {
        std::vector<ast::package_body_declarative_item> res;
        auto items = parse(item, anf);
        res.insert(res.end(), items.begin(), items.end());
        return res;
    }
    if(auto item = ctx->variable_declaration()) {
        std::vector<ast::package_body_declarative_item> res;
        auto items = parse(item, anf);
        res.insert(res.end(), items.begin(), items.end());
        return res;
    }
    if(auto item = ctx->file_declaration()) {
        std::vector<ast::package_body_declarative_item> res;
        auto items = parse(item, anf);
        res.insert(res.end(), items.begin(), items.end());
        return res;
    }
    if(auto item = ctx->alias_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->attribute_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->attribute_specification())
        return {parse(item, anf)};
    if(auto item = ctx->use_clause())
        return {parse(item, anf)};
    if(auto item = ctx->group_template_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->group_declaration())
        return {parse(item, anf)};
    throw std::runtime_error("Unsupported process_or_package_declarative_item");
}

std::vector<ast::process_declarative_item> parse(vhdlParser::Process_declarative_itemContext* ctx, ast::ast_node_factory& anf) {
    if(auto item = ctx->process_or_package_declarative_item()) {
        if(auto decl = item->subprogram_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->subprogram_instantiation_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->package_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->package_instantiation_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->type_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->subtype_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->constant_declaration()) {
            std::vector<ast::package_body_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->variable_declaration()) {
            std::vector<ast::package_body_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->file_declaration()) {
            std::vector<ast::package_body_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->alias_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->attribute_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->attribute_specification())
            return {parse(decl, anf)};
        if(auto decl = item->use_clause())
            return {parse(decl, anf)};
        if(auto decl = item->group_template_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->group_declaration())
            return {parse(decl, anf)};
    }
    if(auto item = ctx->subprogram_body())
        return {parse(item, anf)};
    if(auto item = ctx->package_body())
        return {parse(item, anf)};
    throw std::runtime_error("Unsupported process_declarative_item");
}

std::vector<ast::package_declarative_item> parse(vhdlParser::Package_declarative_itemContext* ctx, ast::ast_node_factory& anf) {
    if(auto item = ctx->process_or_package_declarative_item()) {
        if(auto decl = item->subprogram_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->subprogram_instantiation_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->package_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->package_instantiation_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->type_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->subtype_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->constant_declaration()) {
            std::vector<ast::package_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->variable_declaration()) {
            std::vector<ast::package_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->file_declaration()) {
            std::vector<ast::package_declarative_item> res;
            auto items = parse(decl, anf);
            res.insert(res.end(), items.begin(), items.end());
            return res;
        }
        if(auto decl = item->alias_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->attribute_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->attribute_specification())
            return {parse(decl, anf)};
        if(auto decl = item->use_clause())
            return {parse(decl, anf)};
        if(auto decl = item->group_template_declaration())
            return {parse(decl, anf)};
        if(auto decl = item->group_declaration())
            return {parse(decl, anf)};
    }
    if(auto item = ctx->signal_declaration()) {
        std::vector<ast::package_declarative_item> res;
        auto elems = parse(item, anf);
        res.insert(res.end(), elems.begin(), elems.end());
        return res;
    }
    if(auto item = ctx->component_declaration())
        return {parse(item, anf)};
    if(auto item = ctx->disconnection_specification()) {
        std::vector<ast::package_declarative_item> res;
        auto elems = parse(item, anf);
        res.insert(res.end(), elems.begin(), elems.end());
        return res;
    }
    throw std::runtime_error("Unsupported package_declarative_item");
}

ast::context_reference* parse(vhdlParser::Context_referenceContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::context_reference>();
    auto selected_names = ctx->selected_name();
    node->selected_names.reserve(selected_names.size());
    for(auto selected_name : selected_names) {
        ast::selected_name parsed_name;
        parsed_name.identifier = get_identifier(selected_name->identifier());
        if(!selected_name->suffix().empty()) {
            std::stringstream oss;
            auto suffixes = selected_name->suffix();
            if(suffixes.front()->KW_ALL())
                oss << "all";
            else
                oss << get_literal(suffixes.front()->name_literal());
            for(size_t i = 1; i < suffixes.size(); ++i) {
                if(suffixes[i]->KW_ALL())
                    oss << ".all";
                else
                    oss << "." << get_literal(suffixes[i]->name_literal());
            }
            parsed_name.suffix = oss.str();
        }
        node->selected_names.emplace_back(std::move(parsed_name));
    }
    return node;
}

ast::entity_declaration* parse(vhdlParser::Entity_declarationContext* ctx, ast::ast_node_factory& anf) {
    // entity_declaration:
    //       KW_ENTITY identifier KW_IS
    //           ( generic_clause )?
    //           ( port_clause )?
    //           ( entity_declarative_item )*
    //       ( KW_BEGIN ( entity_statement )* )?
    //       KW_END ( KW_ENTITY )? ( identifier )? SEMI
    // ;
    auto node = anf.create<ast::entity_declaration>();
    node->identifier = ctx->identifier(0)->getText();
    if(auto generic_clause = ctx->generic_clause()) {
        for(auto elem : generic_clause->generic_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
        }
    }
    if(auto port_clause = ctx->port_clause()) {
        for(auto elem : port_clause->port_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->port_list.insert(node->port_list.end(), items.begin(), items.end());
        }
    }
    for(auto e : ctx->entity_declarative_item()) {
        auto elements = parse(e, anf);
        node->entity_declarative_items.insert(node->entity_declarative_items.end(), elements.begin(), elements.end());
    }
    return node;
}

ast::generate_specification_item parse(vhdlParser::Generate_specificationContext* ctx, ast::ast_node_factory& anf) {
    if(auto discrete_range = ctx->discrete_range()) {
        if(auto subtype = discrete_range->subtype_indication())
            return parse(subtype, anf);
        auto range = discrete_range->range();
        if(auto attribute = range->attribute_name())
            return parse(attribute, anf);
        if(auto explicit_range = range->explicit_range())
            return parse(explicit_range, anf);
    }
    if(auto expression = ctx->expression()) {
        auto parsed = parse(expression, anf);
        return std::visit([](auto value) -> ast::generate_specification_item { return value; }, parsed);
    }
    auto label = anf.create<ast::literal_node>();
    label->text = get_identifier(ctx->label()->identifier());
    return label;
}

ast::block_specification* parse(vhdlParser::Block_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::block_specification>();
    node->label = get_identifier(ctx->label()->identifier());
    if(auto generate_spec = ctx->generate_specification())
        node->generate_specification = parse(generate_spec, anf);
    return node;
}

ast::block_configuration* parse(vhdlParser::Block_configurationContext* ctx, ast::ast_node_factory& anf);

ast::component_configuration* parse(vhdlParser::Component_configurationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::component_configuration>();
    node->component_spec = parse(ctx->component_specification(), anf);
    if(auto binding = ctx->binding_indication())
        node->binding = parse(binding, anf);
    if(auto block_config = ctx->block_configuration())
        node->block_config = parse(block_config, anf);
    return node;
}

ast::block_configuration* parse(vhdlParser::Block_configurationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::block_configuration>();
    node->block_spec = parse(ctx->block_specification(), anf);
    const auto& use_clauses = ctx->use_clause();
    node->use_clauses.reserve(use_clauses.size());
    for(auto use_clause : use_clauses)
        node->use_clauses.emplace_back(parse(use_clause, anf));
    const auto& configuration_items = ctx->configuration_item();
    node->configuration_items.reserve(configuration_items.size());
    for(auto item : configuration_items) {
        if(auto nested_block = item->block_configuration())
            node->configuration_items.emplace_back(parse(nested_block, anf));
        else if(auto component = item->component_configuration())
            node->configuration_items.emplace_back(parse(component, anf));
    }
    return node;
}

ast::configuration_declaration* parse(vhdlParser::Configuration_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::configuration_declaration>();
    node->identifier = get_identifier(ctx->identifier(0));
    node->name = ctx->name()->getText();
    const auto& declarative_items = ctx->configuration_declarative_item();
    node->declarative_items.reserve(declarative_items.size());
    for(auto item : declarative_items) {
        if(auto use_clause = item->use_clause())
            node->declarative_items.emplace_back(parse(use_clause, anf));
        else if(auto attribute = item->attribute_specification())
            node->declarative_items.emplace_back(parse(attribute, anf));
        else if(auto group = item->group_declaration())
            node->declarative_items.emplace_back(parse(group, anf));
    }
    node->block_config = parse(ctx->block_configuration(), anf);
    return node;
}

ast::package_declaration* parse(vhdlParser::Package_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::package_declaration>();
    node->identifier = get_identifier(ctx->identifier(0));
    if(auto generic_clause = ctx->generic_clause()) {
        for(auto elem : generic_clause->generic_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
        }
    }
    if(auto generic_map = ctx->generic_map_aspect())
        node->generic_map = parse(generic_map->association_list(), anf);
    for(auto item : ctx->package_declarative_item()) {
        auto elems = parse(item, anf);
        node->declarations.insert(node->declarations.end(), elems.begin(), elems.end());
    }
    return node;
}

ast::package_instantiation_declaration* parse(vhdlParser::Package_instantiation_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::package_instantiation_declaration>();
    node->identifier = get_identifier(ctx->identifier());
    node->target_name = parse(ctx->name(), anf);
    if(auto generic_map = ctx->generic_map_aspect())
        node->generic_map = parse(generic_map->association_list(), anf);
    return node;
}

ast::context_declaration* parse(vhdlParser::Context_declarationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::context_declaration>();
    node->identifier = get_identifier(ctx->identifier(0));
    auto context_items = ctx->context_clause()->context_item();
    node->context_items.reserve(context_items.size());
    for(auto item : context_items) {
        if(auto library_clause = item->library_clause())
            node->context_items.emplace_back(parse(library_clause, anf));
        else if(auto use_clause = item->use_clause())
            node->context_items.emplace_back(parse(use_clause, anf));
        else if(auto context_reference = item->context_reference())
            node->context_items.emplace_back(parse(context_reference, anf));
    }
    return node;
}

ast::block_statement* parse(vhdlParser::Block_statementContext* ctx, ast::ast_node_factory& anf) {
    // block_statement:
    //      KW_BLOCK ( LPAREN condition RPAREN )? ( KW_IS )?
    //          block_header
    //          ( block_declarative_item )*
    //      KW_BEGIN
    //          ( concurrent_statement )*
    //      KW_END KW_BLOCK ( label )? SEMI
    // ;
    auto node = anf.create<ast::block_statement>();
    if(auto condition = ctx->condition())
        node->condition = parse(condition->expression(), anf);
    auto header = ctx->block_header();
    if(auto generic_clause = header->generic_clause()) {
        for(auto elem : generic_clause->generic_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->generic_list.insert(node->generic_list.end(), items.begin(), items.end());
        }
    }
    if(auto port_clause = header->port_clause()) {
        for(auto elem : port_clause->port_list()->interface_list()->interface_element()) {
            auto items = parse(elem, anf);
            node->port_list.insert(node->port_list.end(), items.begin(), items.end());
        }
    }
    if(auto generic_map = header->generic_map_aspect())
        node->generic_map = parse(generic_map->association_list(), anf);
    if(auto port_map = header->port_map_aspect())
        node->port_map = parse(port_map->association_list(), anf);
    if(auto label = ctx->label())
        node->label = get_identifier(label->identifier());
    for(auto b : ctx->block_declarative_item()) {
        auto items = parse(b, anf);
        node->block_declarative_items.insert(node->block_declarative_items.end(), items.begin(), items.end());
    }
    for(auto c : ctx->concurrent_statement())
        node->concurrent_statements.emplace_back(parse(c, anf));
    return node;
}

ast::component_instantiation_statement* parse(vhdlParser::Component_instantiation_statementContext* ctx, ast::ast_node_factory& anf) {
    // component_instantiation_statement:
    //       instantiated_unit
    //           ( generic_map_aspect )?
    //           ( port_map_aspect )? SEMI
    // ;
    auto n = anf.create<ast::component_instantiation_statement>();
    auto unit = ctx->instantiated_unit();
    if(unit->KW_ENTITY()) {
        n->unit_kind = ast::instantiated_unit_kind_e::ENTITY;
        if(auto arch = unit->identifier()) {
            n->architecture_id = get_identifier(arch);
        }
    } else if(unit->KW_CONFIGURATION()) {
        n->unit_kind = ast::instantiated_unit_kind_e::CONFIGURATION;
    } else {
        n->unit_kind = ast::instantiated_unit_kind_e::COMPONENT;
    }
    n->unit_name = get_optional_name(unit->name());
    if(auto generic_map = ctx->generic_map_aspect()) {
        n->generic_map = parse(generic_map->association_list(), anf);
    }
    if(auto port_map = ctx->port_map_aspect()) {
        n->port_map = parse(port_map->association_list(), anf);
    }
    return n;
}

ast::for_generate_statement* parse(vhdlParser::For_generate_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::for_generate_statement>();
    auto parameter = ctx->parameter_specification();
    n->parameter = get_identifier(parameter->identifier());
    n->range = parameter->discrete_range()->getText();
    parse(ctx->generate_statement_body(), n->body, anf);
    return n;
}

ast::if_generate_statement* parse(vhdlParser::If_generate_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::if_generate_statement>();
    auto labels = ctx->label();
    auto conditions = ctx->condition();
    auto bodies = ctx->generate_statement_body();
    auto previous_token = static_cast<long long>(ctx->KW_IF()->getSymbol()->getTokenIndex());
    for(size_t i = 0; i < conditions.size(); ++i) {
        auto clause = anf.create<ast::if_generate_clause>();
        clause->condition = parse(conditions[i], anf);
        clause->label = find_label_between(labels, previous_token, conditions[i]->getStart()->getTokenIndex());
        if(i < bodies.size()) {
            parse(bodies[i], clause->body, anf);
            previous_token = bodies[i]->getStop()->getTokenIndex();
        } else {
            previous_token = conditions[i]->getStop()->getTokenIndex();
        }
        n->clauses.emplace_back(clause);
    }
    if(bodies.size() > conditions.size()) {
        n->has_else = true;
        auto else_token = ctx->KW_ELSE()->getSymbol()->getTokenIndex();
        auto else_body = bodies.back();
        n->else_label = find_label_between(labels, else_token, else_body->getStart()->getTokenIndex());
        parse(else_body, n->else_body, anf);
    }
    return n;
}

ast::case_generate_statement* parse(vhdlParser::Case_generate_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::case_generate_statement>();
    n->expression = ctx->expression()->getText();
    for(auto alt : ctx->case_generate_alternative()) {
        auto parsed_alt = anf.create<ast::case_generate_alternative>();
        parsed_alt->choices = parse(alt->choices(), anf);
        if(auto label = alt->label()) {
            parsed_alt->label = get_identifier(label->identifier());
        }
        parse(alt->generate_statement_body_with_begin_end(), parsed_alt->body, anf);
        n->alternatives.emplace_back(parsed_alt);
    }
    return n;
}

std::variant<ast::for_generate_statement*, ast::if_generate_statement*, ast::case_generate_statement*>
parse(vhdlParser::Generate_statementContext* ctx, ast::ast_node_factory& anf) {
    // generate_statement:
    //     for_generate_statement
    //     | if_generate_statement
    //     | case_generate_statement
    // ;
    if(auto s = ctx->for_generate_statement()) {
        return parse(s, anf);
    }
    if(auto s = ctx->if_generate_statement()) {
        return parse(s, anf);
    }
    if(auto s = ctx->case_generate_statement()) {
        return parse(s, anf);
    }
    throw std::runtime_error("Unsupported generate_statement alternative");
}

ast::target_item parse(vhdlParser::TargetContext* ctx, ast::ast_node_factory& anf) {
    if(auto aggregate = ctx->aggregate())
        return parse(aggregate, anf);
    return parse(ctx->name(), anf);
}

ast::force_mode_e parse(vhdlParser::Force_modeContext* ctx) {
    return ctx && ctx->KW_OUT() ? ast::force_mode_e::OUT : ast::force_mode_e::IN;
}

template <typename T> void parse_delay_mechanism(vhdlParser::Delay_mechanismContext* ctx, T* node, ast::ast_node_factory& anf) {
    if(!ctx)
        return;
    node->delay_mechanism_type = ctx->KW_INERTIAL() ? ast::delay_mechanism_e::INERTIAL : ast::delay_mechanism_e::TRANSPORT;
    if(ctx->KW_REJECT())
        node->delay_mechanism_reject = parse(ctx->expression(), anf);
}

std::vector<ast::conditional_expression_element*> parse(vhdlParser::Conditional_expressionsContext* ctx, ast::ast_node_factory& anf) {
    const auto& expressions = ctx->expression();
    const auto& conditions = ctx->condition();

    std::vector<ast::conditional_expression_element*> elements;
    elements.reserve(expressions.size());

    const auto conditional_count = conditions.size();
    for(size_t i = 0; i < conditional_count; ++i) {
        auto element = anf.create<ast::conditional_expression_element>();
        element->value = parse(expressions[i], anf);
        element->condition = parse(conditions[i]->expression(), anf);
        elements.emplace_back(element);
    }

    if(expressions.size() > conditional_count) {
        auto element = anf.create<ast::conditional_expression_element>();
        element->value = parse(expressions[conditional_count], anf);
        elements.emplace_back(element);
    }

    return elements;
}

std::vector<ast::selected_expression_element*> parse(vhdlParser::Selected_expressionsContext* ctx, ast::ast_node_factory& anf) {
    const auto& expressions = ctx->expression();
    const auto& choices = ctx->choices();

    std::vector<ast::selected_expression_element*> elements;
    elements.reserve(expressions.size());

    const auto count = std::min(expressions.size(), choices.size());
    for(size_t i = 0; i < count; ++i) {
        auto element = anf.create<ast::selected_expression_element>();
        element->waveform.value = parse(expressions[i], anf);
        element->choices = parse(choices[i], anf);
        elements.emplace_back(element);
    }

    return elements;
}

std::vector<ast::waveform_element> parse_waveform_values(vhdlParser::WaveformContext* ctx, ast::ast_node_factory& anf) {
    auto parsed = parse(ctx, anf);
    std::vector<ast::waveform_element> values;
    values.reserve(parsed.size());
    for(auto element : parsed)
        values.emplace_back(*element);
    return values;
}

std::vector<ast::conditional_waveform_element> parse_conditional_waveform_values(vhdlParser::Conditional_waveformsContext* ctx,
                                                                                 ast::ast_node_factory& anf) {
    auto parsed = parse(ctx, anf);
    std::vector<ast::conditional_waveform_element> values;
    values.reserve(parsed.size());
    for(auto element : parsed)
        values.emplace_back(*element);
    return values;
}

ast::sequential_statement_item parse(vhdlParser::Wait_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::wait_statement>();
    if(auto sensitivity = ctx->sensitivity_clause()) {
        const auto& names = sensitivity->sensitivity_list()->name();
        node->sensitivity_list.reserve(names.size());
        for(auto name : names)
            node->sensitivity_list.emplace_back(get_optional_name(name));
    }
    if(auto condition = ctx->condition_clause())
        node->condition_clause = parse(condition->condition()->expression(), anf);
    if(auto timeout = ctx->timeout_clause())
        node->timeout_clause = parse(timeout->expression(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Assertion_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::assertion_statement>();
    auto assertion = ctx->assertion();
    node->condition = parse(assertion->condition()->expression(), anf);
    const auto& expressions = assertion->expression();
    if(assertion->KW_REPORT() && !expressions.empty())
        node->report = parse(expressions[0], anf);
    if(assertion->KW_SEVERITY() && !expressions.empty())
        node->severity = parse(expressions[assertion->KW_REPORT() && expressions.size() > 1 ? 1 : 0], anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Report_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::report_statement>();
    const auto& expressions = ctx->expression();
    node->report = parse(expressions[0], anf);
    if(ctx->KW_SEVERITY() && expressions.size() > 1)
        node->severity = parse(expressions[1], anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Simple_waveform_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::simple_waveform_assignment>();
    node->target = parse(ctx->target(), anf);
    parse_delay_mechanism(ctx->delay_mechanism(), node, anf);
    node->waveform = parse_waveform_values(ctx->waveform(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Simple_force_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::simple_force_assignment>();
    node->target = parse(ctx->target(), anf);
    node->force_mode = parse(ctx->force_mode());
    auto element = ast::waveform_element{};
    element.value = parse(ctx->expression(), anf);
    node->waveform.emplace_back(element);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Simple_release_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::simple_release_assignment>();
    node->target = parse(ctx->target(), anf);
    node->force_mode = parse(ctx->force_mode());
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Conditional_waveform_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::conditional_waveform_assignment>();
    node->target = parse(ctx->target(), anf);
    parse_delay_mechanism(ctx->delay_mechanism(), node, anf);
    node->conditional_waveforms = parse_conditional_waveform_values(ctx->conditional_waveforms(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Conditional_force_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::conditional_force_assignment>();
    node->target = parse(ctx->target(), anf);
    node->force_mode = parse(ctx->force_mode());
    node->conditional_expressions = parse(ctx->conditional_expressions(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Selected_waveform_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::selected_waveform_assignment>();
    node->with_expression = parse(ctx->expression(), anf);
    node->is_questionable = ctx->QUESTIONMARK();
    node->target = parse(ctx->target(), anf);
    parse_delay_mechanism(ctx->delay_mechanism(), node, anf);
    node->selected_waveforms = parse(ctx->selected_waveforms(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Selected_force_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::selected_force_assignment>();
    node->with_expression = parse(ctx->expression(), anf);
    node->is_questionable = ctx->QUESTIONMARK();
    node->target = parse(ctx->target(), anf);
    node->force_mode = parse(ctx->force_mode());
    node->selected_expressions = parse(ctx->selected_expressions(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Signal_assignment_statementContext* ctx, ast::ast_node_factory& anf) {
    if(auto simple = ctx->simple_signal_assignment()) {
        if(auto s = simple->simple_waveform_assignment())
            return parse(s, anf);
        if(auto s = simple->simple_force_assignment())
            return parse(s, anf);
        if(auto s = simple->simple_release_assignment())
            return parse(s, anf);
    } else if(auto conditional = ctx->conditional_signal_assignment()) {
        if(auto s = conditional->conditional_waveform_assignment())
            return parse(s, anf);
        if(auto s = conditional->conditional_force_assignment())
            return parse(s, anf);
    } else if(auto selected = ctx->selected_signal_assignment()) {
        if(auto s = selected->selected_waveform_assignment())
            return parse(s, anf);
        if(auto s = selected->selected_force_assignment())
            return parse(s, anf);
    }
    throw std::runtime_error("Unsupported signal_assignment_statement alternative");
}

ast::sequential_statement_item parse(vhdlParser::Simple_variable_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::simple_variable_assignment>();
    node->target = parse(ctx->target(), anf);
    node->value = parse(ctx->expression(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Conditional_variable_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::conditional_variable_assignment>();
    node->target = parse(ctx->target(), anf);
    node->conditional_expressions = parse(ctx->conditional_expressions(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Selected_variable_assignmentContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::selected_variable_assignment>();
    node->with_expression = parse(ctx->expression(), anf);
    node->is_questionable = ctx->QUESTIONMARK();
    node->target = parse(ctx->target(), anf);
    node->selected_expressions = parse(ctx->selected_expressions(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Variable_assignment_statementContext* ctx, ast::ast_node_factory& anf) {
    if(auto s = ctx->simple_variable_assignment())
        return parse(s, anf);
    if(auto s = ctx->conditional_variable_assignment())
        return parse(s, anf);
    if(auto s = ctx->selected_variable_assignment())
        return parse(s, anf);
    throw std::runtime_error("Unsupported variable_assignment_statement alternative");
}

ast::sequential_statement_item parse(vhdlParser::Procedure_call_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::procedure_call_statement>();
    node->name = get_optional_name(ctx->procedure_call()->name());
    return node;
}

ast::sequential_statement_item parse(vhdlParser::If_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::if_statement>();
    const auto& conditions = ctx->condition();
    const auto& statement_blocks = ctx->sequence_of_statements();
    node->if_clauses.reserve(conditions.size());
    for(size_t i = 0; i < conditions.size(); ++i)
        node->if_clauses.emplace_back(parse(conditions[i]->expression(), anf), parse(statement_blocks[i], anf));
    if(ctx->KW_ELSE() && statement_blocks.size() > conditions.size())
        node->else_clauses = parse(statement_blocks[conditions.size()], anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Case_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::case_statement>();
    node->expression = parse(ctx->expression(), anf);
    const auto& alternatives = ctx->case_statement_alternative();
    node->alternatives.reserve(alternatives.size());
    for(auto alternative : alternatives) {
        ast::case_statement_alternative parsed;
        parsed.choices = parse(alternative->choices(), anf);
        parsed.statements = parse(alternative->sequence_of_statements(), anf);
        node->alternatives.emplace_back(std::move(parsed));
    }
    return node;
}

ast::parameter_specification* parse(vhdlParser::Parameter_specificationContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::parameter_specification>();
    node->for_identifier = get_identifier(ctx->identifier());
    node->range = parse(ctx->discrete_range(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Loop_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::loop_statement>();
    if(auto iteration = ctx->iteration_scheme()) {
        if(auto condition = iteration->condition())
            node->iteration_scheme = parse(condition->expression(), anf);
        else if(auto parameter = iteration->parameter_specification())
            node->iteration_scheme = parse(parameter, anf);
    }
    node->statements = parse(ctx->sequence_of_statements(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Next_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::next_statement>();
    if(auto label = ctx->label())
        node->label = get_identifier(label->identifier());
    if(auto condition = ctx->condition())
        node->exit_expression = parse(condition->expression(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Exit_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::exit_statement>();
    if(auto label = ctx->label())
        node->label = get_identifier(label->identifier());
    if(auto condition = ctx->condition())
        node->exit_expression = parse(condition->expression(), anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Return_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::return_statement>();
    if(auto expression = ctx->expression())
        node->return_expression = parse(expression, anf);
    return node;
}

ast::sequential_statement_item parse(vhdlParser::Null_statementContext*, ast::ast_node_factory& anf) {
    return anf.create<ast::null_statement>();
}

ast::sequential_statement_item parse_statement(vhdlParser::Sequential_statementContext* ctx, ast::ast_node_factory& anf) {
    if(auto s = ctx->wait_statement())
        return parse(s, anf);
    if(auto s = ctx->assertion_statement())
        return parse(s, anf);
    if(auto s = ctx->report_statement())
        return parse(s, anf);
    if(auto s = ctx->signal_assignment_statement())
        return parse(s, anf);
    if(auto s = ctx->variable_assignment_statement())
        return parse(s, anf);
    if(auto s = ctx->procedure_call_statement())
        return parse(s, anf);
    if(auto s = ctx->if_statement())
        return parse(s, anf);
    if(auto s = ctx->case_statement())
        return parse(s, anf);
    if(auto s = ctx->loop_statement())
        return parse(s, anf);
    if(auto s = ctx->next_statement())
        return parse(s, anf);
    if(auto s = ctx->exit_statement())
        return parse(s, anf);
    if(auto s = ctx->return_statement())
        return parse(s, anf);
    if(auto s = ctx->null_statement())
        return parse(s, anf);
    throw std::runtime_error("Unsupported sequential_statement alternative");
}

ast::sequential_statement* parse(vhdlParser::Sequential_statementContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::sequential_statement>();
    if(auto label = ctx->label())
        node->label = get_identifier(label->identifier());
    node->stmt = parse_statement(ctx, anf);
    return node;
}

std::vector<ast::sequential_statement_item> parse(vhdlParser::Sequence_of_statementsContext* ctx, ast::ast_node_factory& anf) {
    const auto& statements = ctx->sequential_statement();
    std::vector<ast::sequential_statement_item> parsed;
    parsed.reserve(statements.size());
    for(auto statement : statements)
        parsed.emplace_back(parse_statement(statement, anf));
    return parsed;
}

ast::process_statement* parse(vhdlParser::Process_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::process_statement>();
    n->postponed = !ctx->KW_POSTPONED().empty();
    if(auto sensitivity = ctx->process_sensitivity_list()) {
        if(sensitivity->KW_ALL()) {
            n->sensitivity_all = true;
        } else if(auto list = sensitivity->sensitivity_list()) {
            for(auto name : list->name()) {
                n->sensitivity_list.emplace_back(get_optional_name(name));
            }
        }
    }
    auto declarative_items = ctx->process_declarative_item();
    n->declarative_items.reserve(declarative_items.size());
    for(auto d : declarative_items) {
        auto items = parse(d, anf);
        n->declarative_items.insert(n->declarative_items.end(), items.begin(), items.end());
    }
    for(auto s : ctx->sequential_statement()) {
        n->sequential_statements.emplace_back(parse(s, anf));
    }
    return n;
}

ast::concurrent_procedure_call_statement* parse(vhdlParser::Concurrent_procedure_call_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::concurrent_procedure_call_statement>();
    n->postponed = ctx->KW_POSTPONED();
    n->name = get_optional_name(ctx->procedure_call_statement()->procedure_call()->name());
    return n;
}

ast::concurrent_assertion_statement* parse(vhdlParser::Concurrent_assertion_statementContext* ctx, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::concurrent_assertion_statement>();
    n->postponed = ctx->KW_POSTPONED();
    auto assertion = ctx->assertion_statement()->assertion();
    n->condition = parse(assertion->condition()->expression(), anf);
    auto expressions = assertion->expression();
    if(assertion->KW_REPORT() && !expressions.empty()) {
        n->report = parse(expressions[0], anf);
    }
    if(assertion->KW_SEVERITY() && !expressions.empty()) {
        n->severity = parse(expressions[assertion->KW_REPORT() && expressions.size() > 1 ? 1 : 0], anf);
    }
    return n;
}

std::vector<ast::waveform_element*> parse(vhdlParser::WaveformContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::waveform_element*> elements;
    const auto& waveform_elements = ctx->waveform_element();
    elements.reserve(waveform_elements.size());
    for(auto waveform_element_ctx : waveform_elements) {
        auto element = anf.create<ast::waveform_element>();
        if(auto value = waveform_element_ctx->expression(0))
            element->value = parse(value, anf);
        if(auto after = waveform_element_ctx->expression(1))
            element->after_expression = parse(after, anf);
        elements.emplace_back(element);
    }
    return elements;
}

std::vector<ast::conditional_waveform_element*> parse(vhdlParser::Conditional_waveformsContext* ctx, ast::ast_node_factory& anf) {
    const auto& waveforms = ctx->waveform();
    const auto& conditions = ctx->condition();

    std::vector<ast::conditional_waveform_element*> elements;
    elements.reserve(waveforms.size());

    const auto conditional_count = conditions.size();
    for(size_t i = 0; i < conditional_count; ++i) {
        auto element = anf.create<ast::conditional_waveform_element>();
        element->waveform = parse(waveforms[i], anf);
        element->condition = parse(conditions[i]->expression(), anf);
        elements.emplace_back(element);
    }

    if(waveforms.size() > conditional_count) {
        auto element = anf.create<ast::conditional_waveform_element>();
        element->waveform = parse(waveforms[conditional_count], anf);
        elements.emplace_back(element);
    }

    return elements;
}

std::vector<ast::selected_waveform_element*> parse(vhdlParser::Selected_waveformsContext* ctx, ast::ast_node_factory& anf) {
    const auto& waveforms = ctx->waveform();
    const auto& choices = ctx->choices();

    std::vector<ast::selected_waveform_element*> elements;
    elements.reserve(waveforms.size());

    const auto count = std::min(waveforms.size(), choices.size());
    for(size_t i = 0; i < count; ++i) {
        auto element = anf.create<ast::selected_waveform_element>();
        element->waveform = parse(waveforms[i], anf);

        auto parsed_choices = parse(choices[i], anf);
        element->choices.reserve(parsed_choices.size());
        for(auto choice : parsed_choices)
            element->choices.emplace_back(choice);

        elements.emplace_back(element);
    }

    return elements;
}

ast::concurrent_signal_assignment_any* parse(vhdlParser::Concurrent_signal_assignment_anyContext* any, ast::ast_node_factory& anf) {
    auto n = anf.create<ast::concurrent_signal_assignment_any>();
    // n->postponed = ctx->KW_POSTPONED();
    n->target = parse(any->target(), anf);
    n->guarded = any->KW_GUARDED();
    if(auto dm = any->delay_mechanism()) {
        n->delay_mechanism_type = dm->KW_INERTIAL() ? ast::delay_mechanism_e::INERTIAL : ast::delay_mechanism_e::TRANSPORT;
        if(dm->KW_REJECT())
            n->delay_mechanism_reject = parse(any->delay_mechanism()->expression(), anf);
    }
    if(auto waveform = any->waveform()) {
        n->kind = ast::concurrent_signal_assignment_kind_e::SIMPLE;
        n->waveform = parse(waveform, anf);
    } else if(auto conditional = any->conditional_waveforms()) {
        n->kind = ast::concurrent_signal_assignment_kind_e::CONDITIONAL;
        n->conditional_waveforms = parse(conditional, anf);
    }
    return n;
}

ast::concurrent_selected_signal_assignment* parse(vhdlParser::Concurrent_selected_signal_assignmentContext* selected,
                                                  ast::ast_node_factory& anf) {
    auto n = anf.create<ast::concurrent_selected_signal_assignment>();
    // n->postponed = ctx->KW_POSTPONED();
    n->with_expression = parse(selected->expression(), anf);
    n->target = parse(selected->target(), anf);
    n->guarded = selected->KW_GUARDED();
    if(auto delay_mechanism = selected->delay_mechanism()) {
        n->delay_mechanism_type = delay_mechanism->KW_INERTIAL() ? ast::delay_mechanism_e::INERTIAL : ast::delay_mechanism_e::TRANSPORT;
        if(delay_mechanism->KW_REJECT())
            n->delay_mechanism_reject = parse(delay_mechanism->expression(), anf);
    }
    if(auto selected_waveforms = selected->selected_waveforms())
        n->selected_waveforms = parse(selected_waveforms, anf);
    return n;
}

ast::concurrent_statement* parse(vhdlParser::Concurrent_statement_with_optional_labelContext* ctx, ast::ast_node_factory& anf) {
    // concurrent_statement_with_optional_label:
    //       process_statement
    //       | concurrent_procedure_call_statement
    //       | concurrent_assertion_statement
    //       | concurrent_signal_assignment_statement
    // ;
    auto stmt = anf.create<ast::concurrent_statement>();
    if(auto s = ctx->process_statement()) {
        stmt->statement = parse(s, anf);
    }
    if(auto s = ctx->concurrent_procedure_call_statement()) {
        stmt->statement = parse(s, anf);
    }
    if(auto s = ctx->concurrent_assertion_statement()) {
        stmt->statement = parse(s, anf);
    }
    if(auto s = ctx->concurrent_signal_assignment_statement()) {
        if(auto any = s->concurrent_signal_assignment_any()) {
            auto n = parse(any, anf);
            n->postponed = s->KW_POSTPONED();
            stmt->statement = n;
        } else if(auto selected = s->concurrent_selected_signal_assignment()) {
            auto n = parse(selected, anf);
            n->postponed = s->KW_POSTPONED();
            stmt->statement = n;
        }
    }
    return stmt;
}

ast::concurrent_statement* parse(vhdlParser::Concurrent_statementContext* ctx, ast::ast_node_factory& anf) {
    // concurrent_statement:
    //       label COLON (block_statement
    //                    | component_instantiation_statement
    //                    | generate_statement
    //                    | concurrent_statement_with_optional_label)
    //       | concurrent_statement_with_optional_label
    // ;
    ast::concurrent_statement* n = anf.create<ast::concurrent_statement>();
    if(auto label = ctx->label()) {
        n->label = get_identifier(label->identifier());
    }
    if(auto s = ctx->block_statement()) {
        n->statement = parse(s, anf);
    } else if(auto s = ctx->component_instantiation_statement()) {
        n->statement = parse(s, anf);
    } else if(auto s = ctx->generate_statement()) {
        std::visit([n](auto stmt) { n->statement = stmt; }, parse(s, anf));
    } else if(auto s = ctx->concurrent_statement_with_optional_label()) {
        n->statement = parse(s, anf)->statement;
    }
    return n;
}

ast::architecture_body* parse(vhdlParser::Architecture_bodyContext* ctx, ast::ast_node_factory& anf) {
    // architecture_body:
    //       KW_ARCHITECTURE identifier KW_OF name KW_IS
    //           ( block_declarative_item )*
    //       KW_BEGIN
    //           ( concurrent_statement )*
    //       KW_END ( KW_ARCHITECTURE )? ( identifier )? SEMI
    // ;
    auto n = anf.create<ast::architecture_body>();
    n->identifier = ctx->identifier(0)->getText();
    n->primary = parse(ctx->name(), anf);
    for(auto b : ctx->block_declarative_item()) {
        auto items = parse(b, anf);
        n->block_declarative_items.insert(n->block_declarative_items.end(), items.begin(), items.end());
    }
    for(auto c : ctx->concurrent_statement()) {
        n->concurrent_statements.emplace_back(parse(c, anf));
    }
    return n;
}

ast::subprogram_body* parse(vhdlParser::Subprogram_bodyContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::subprogram_body>();
    node->specification = parse(ctx->subprogram_specification(), anf);
    auto x = parse(ctx->subprogram_specification(), anf);
    for(auto item : ctx->process_declarative_item()) {
        for(auto parsed : parse(item, anf))
            std::visit([node](auto value) { node->declarative_items.emplace_back(value); }, parsed);
    }
    for(auto stmt : ctx->sequential_statement())
        node->sequential_statements.emplace_back(parse(stmt, anf));
    return node;
}

ast::package_body* parse(vhdlParser::Package_bodyContext* ctx, ast::ast_node_factory& anf) {
    auto node = anf.create<ast::package_body>();
    node->identifier = get_identifier(ctx->identifier(0));
    for(auto item : ctx->process_declarative_item()) {
        for(auto parsed : parse(item, anf))
            node->declarative_items.emplace_back(parsed);
    }
    return node;
}

ast::constraint_item parse(vhdlParser::ConstraintContext* ctx, ast::ast_node_factory& anf) {
    if(auto range_constraint = ctx->range_constraint()) {
        auto range = range_constraint->range();
        if(auto attribute = range->attribute_name())
            return parse(attribute, anf);
        if(auto explicit_range = range->explicit_range())
            return parse(explicit_range, anf);
    } else if(auto element_constraint = ctx->element_constraint()) {
        return parse(element_constraint, anf);
    }

    throw std::runtime_error("Unsupported constraint");
}

std::vector<ast::signal_declaration*> parse(vhdl_antlr::vhdlParser::Signal_declarationContext* ctx, ast::ast_node_factory& anf) {
    std::vector<ast::signal_declaration*> res;
    for(auto name : ctx->identifier_list()->identifier()) {
        auto node = anf.create<ast::signal_declaration>();
        node->identifier = get_identifier(name);
        auto subtype = ctx->subtype_indication();
        if(auto resolution = subtype->resolution_indication())
            node->resolution = parse(resolution, anf);
        node->type = parse(subtype->type_mark(), anf);
        if(auto constraint = subtype->constraint())
            node->constraint = parse(constraint, anf);
        node->is_bus = ctx->signal_kind() && ctx->signal_kind()->KW_BUS();
        node->mode = ast::signal_mode_e::NONE;
        res.push_back(node);
    }
    return res;
}

} // namespace context
} // namespace parser
