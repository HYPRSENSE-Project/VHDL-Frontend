#pragma once

#include "ast_nodes.h"
#include <type_traits>

namespace ast {

namespace detail {
inline std::string to_string(const ast::primary_item r);
inline std::string to_string(const ast::expression_item r);
inline std::string to_string(const ast::explicit_range* r);
inline std::string to_string(const ast::name_node* n);

inline const char* to_string(direction_e direction) noexcept {
    switch(direction) {
    case direction_e::TO:
        return "to";
    case direction_e::DOWNTO:
        return "downto";
    }
    return "";
}

inline const char* to_string(operation_kind_e op) noexcept {
    switch(op) {
    case operation_kind_e::NONE:
        return "";
    case operation_kind_e::PLUS:
        return "+";
    case operation_kind_e::MINUS:
        return "-";
    case operation_kind_e::AMPERSAND:
        return "&";
    case operation_kind_e::MUL:
        return "*";
    case operation_kind_e::DIV:
        return "/";
    case operation_kind_e::MOD:
        return "mod";
    case operation_kind_e::REM:
        return "rem";
    case operation_kind_e::DOUBLESTAR:
        return "**";
    case operation_kind_e::ABS:
        return "abs";
    case operation_kind_e::NOT:
        return "not";
    case operation_kind_e::SIGN_PLUS:
        return "+";
    case operation_kind_e::SIGN_MINUS:
        return "-";
    case operation_kind_e::AND:
        return "and";
    case operation_kind_e::OR:
        return "or";
    case operation_kind_e::NAND:
        return "nand";
    case operation_kind_e::NOR:
        return "nor";
    case operation_kind_e::XOR:
        return "xor";
    case operation_kind_e::XNOR:
        return "xnor";
    case operation_kind_e::SLL:
        return "sll";
    case operation_kind_e::SRL:
        return "srl";
    case operation_kind_e::SLA:
        return "sla";
    case operation_kind_e::SRA:
        return "sra";
    case operation_kind_e::ROL:
        return "rol";
    case operation_kind_e::ROR:
        return "ror";
    case operation_kind_e::EQ:
        return "=";
    case operation_kind_e::NE:
        return "/=";
    case operation_kind_e::LT:
        return "<";
    case operation_kind_e::CONASGN:
        return "<=";
    case operation_kind_e::GT:
        return ">";
    case operation_kind_e::GE:
        return ">=";
    case operation_kind_e::EQ_MATCH:
        return "?=";
    case operation_kind_e::NE_MATCH:
        return "?/=";
    case operation_kind_e::LT_MATCH:
        return "?<";
    case operation_kind_e::LE_MATCH:
        return "?<=";
    case operation_kind_e::GT_MATCH:
        return "?>";
    case operation_kind_e::GE_MATCH:
        return "?>=";
    }
    return "";
}

inline std::string to_string(const ast::type_mark* type) { return type && type->name ? to_string(type->name) : std::string{}; }

inline std::string to_string(const ast::signature* signature) {
    if(!signature)
        return {};
    std::string ret{"["};
    bool has_entry = false;
    for(const auto* type_mark : signature->type_marks) {
        if(has_entry)
            ret += ", ";
        ret += to_string(type_mark);
        has_entry = true;
    }
    if(signature->return_type_mark) {
        if(has_entry)
            ret += " ";
        ret += "return ";
        ret += to_string(signature->return_type_mark);
    }
    ret.push_back(']');
    return ret;
}

inline std::string join_binary(std::string lhs, const char* op, std::string rhs) {
    if(lhs.empty())
        return rhs;
    if(rhs.empty())
        return lhs;
    std::string ret;
    ret.reserve(lhs.size() + rhs.size() + 2 + std::char_traits<char>::length(op));
    ret += lhs;
    ret.push_back(' ');
    ret += op;
    ret.push_back(' ');
    ret += rhs;
    return ret;
}

inline std::string to_string(const ast::explicit_range* r) {
    if(!r)
        return {};
    return join_binary(to_string(r->left), to_string(r->direction), to_string(r->right));
}

inline std::string to_string(const ast::subtype_indication* si) {
    if(!si)
        return {};
    std::string ret;
    if(si->resolution && !si->resolution->name.empty()) {
        ret += si->resolution->name;
        ret.push_back(' ');
    }
    ret += to_string(si->type);
    return ret;
}

inline std::string to_string(const ast::actual_designator* designator) {
    if(!designator)
        return {};
    if(designator->is_open)
        return "open";
    std::string ret;
    if(designator->is_inertial)
        ret = "inertial ";
    if(designator->indication)
        ret += to_string(designator->indication);
    else
        ret += to_string(designator->expr);
    return ret;
}

inline std::string to_string(const ast::association_element* assoc) {
    if(!assoc)
        return {};
    std::string actual = assoc->actual_name;
    auto designator = to_string(assoc->act_designator);
    if(!actual.empty() && !designator.empty()) {
        actual.push_back('(');
        actual += designator;
        actual.push_back(')');
    } else if(actual.empty()) {
        actual = designator;
    }
    if(assoc->formal_name.empty())
        return actual;

    std::string ret = assoc->formal_name;
    if(!assoc->formal_paren_name.empty()) {
        ret.push_back('(');
        ret += assoc->formal_paren_name;
        ret.push_back(')');
    }
    ret += " => ";
    ret += actual;
    return ret;
}

inline std::string to_string(const ast::aggregate* aggr) {
    if(!aggr)
        return {};
    std::string ret;
    ret.push_back('(');
    for(std::size_t i = 0; i < aggr->elem_assoc.size(); ++i) {
        if(i)
            ret += ", ";
        auto* assoc = aggr->elem_assoc[i];
        if(!assoc)
            continue;
        if(!assoc->choices.empty()) {
            for(std::size_t j = 0; j < assoc->choices.size(); ++j) {
                if(j)
                    ret += " | ";
                ret += to_string(ast::expression_item{assoc->choices[j]});
            }
            ret += " => ";
        }
        ret += to_string(assoc->expr);
    }
    ret.push_back(')');
    return ret;
}

inline std::string to_string(const ast::qualified_expression* qe) {
    if(!qe)
        return {};
    auto type = to_string(qe->type);
    auto aggregate = to_string(qe->aggr);
    if(type.empty())
        return aggregate;
    std::string ret;
    ret.reserve(type.size() + aggregate.size() + 1);
    ret += type;
    ret.push_back('\'');
    ret += aggregate;
    return ret;
}

inline std::string to_string(const ast::allocator* alloc) {
    if(!alloc)
        return {};
    std::string value = alloc->si ? to_string(alloc->si) : to_string(alloc->qe);
    if(value.empty())
        return "new";
    std::string ret;
    ret.reserve(value.size() + 4);
    ret = "new ";
    ret += value;
    return ret;
}

inline std::string to_string(const ast::simple_expression* node) {
    if(!node)
        return {};

    switch(node->kind) {
    case simple_expression_kind_e::PRIMARY:
        return to_string(node->first_primary);
    case simple_expression_kind_e::POWER:
        return join_binary(to_string(node->first_primary), to_string(node->op), to_string(node->secondary_primary));
    case simple_expression_kind_e::PREFIX: {
        auto operand = to_string(ast::expression_item{node->operand});
        const auto* op = to_string(node->op);
        if(operand.empty())
            return {};
        if(node->op == operation_kind_e::SIGN_PLUS || node->op == operation_kind_e::SIGN_MINUS)
            return std::string(op) + operand;
        std::string ret;
        ret.reserve(operand.size() + std::char_traits<char>::length(op) + 1);
        ret += op;
        ret.push_back(' ');
        ret += operand;
        return ret;
    }
    case simple_expression_kind_e::MUL_DIV:
    case simple_expression_kind_e::ADD_SUB:
        return join_binary(to_string(ast::expression_item{node->lhs}), to_string(node->op), to_string(ast::expression_item{node->rhs}));
    case simple_expression_kind_e::RAW:
        if(auto value = to_string(node->first_primary); !value.empty())
            return value;
        if(node->operand)
            return to_string(ast::expression_item{node->operand});
        if(node->lhs || node->rhs)
            return join_binary(to_string(ast::expression_item{node->lhs}), to_string(node->op), to_string(ast::expression_item{node->rhs}));
        return {};
    }
    return {};
}

inline std::string to_string(const ast::binary_expression* node) {
    if(!node)
        return {};
    return join_binary(to_string(node->lhs), to_string(node->op), to_string(node->rhs));
}

inline std::string to_string(ast::primary_item r) {
    return std::visit(
        [](auto* node) -> std::string {
            using T = std::remove_pointer_t<decltype(node)>;
            if(!node)
                return {};
            if constexpr(std::is_same_v<T, ast::literal_node>) {
                return node->text;
            } else if constexpr(std::is_same_v<T, ast::name_node>) {
                return to_string(static_cast<ast::name_node*>(node));
            } else if constexpr(std::is_same_v<T, ast::allocator>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::aggregate>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::qualified_expression>) {
                return to_string(node);
            }
            return {};
        },
        r);
}

inline std::string to_string(const ast::expression_item r) {
    return std::visit(
        [](auto* node) -> std::string {
            using T = std::remove_pointer_t<decltype(node)>;
            if(!node)
                return {};
            if constexpr(std::is_same_v<T, ast::simple_expression>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::conditional_primary>) {
                auto value = detail::to_string(node->prim);
                if(value.empty())
                    return {};
                std::string ret;
                ret.reserve(value.size() + 3);
                ret = "?? ";
                ret += value;
                return ret;
            } else if constexpr(std::is_same_v<T, ast::literal_node>) {
                return node->text;
            } else if constexpr(std::is_same_v<T, ast::allocator>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::aggregate>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::qualified_expression>) {
                return to_string(node);
            } else if constexpr(std::is_same_v<T, ast::binary_expression>) {
                return to_string(node);
            }
            return {};
        },
        r);
}

inline std::string to_string(const ast::name_node* n) {
    if(!n)
        return {};

    switch(n->kind) {
    case ast::name_kind_e::SIMPLE:
    case ast::name_kind_e::EXTERNAL:
        return n->value;
    case ast::name_kind_e::SELECTED: {
        auto prefix = to_string(n->prefix);
        if(prefix.empty())
            return n->value;
        std::string ret;
        ret.reserve(prefix.size() + n->value.size() + 1);
        ret += prefix;
        ret.push_back('.');
        ret += n->value;
        return ret;
    }
    case ast::name_kind_e::SLICE: {
        auto prefix = to_string(n->prefix);
        auto range = n->slice ? to_string(n->slice->range) : std::string{};
        std::string ret;
        ret.reserve(prefix.size() + range.size() + 2);
        ret += prefix;
        ret.push_back('(');
        ret += range;
        ret.push_back(')');
        return ret;
    }
    case ast::name_kind_e::ATTRIBUTE: {
        auto prefix = to_string(n->prefix);
        std::string ret = prefix;
        if(n->attribute) {
            ret += to_string(n->attribute->signatue);
            ret.push_back('\'');
            ret += n->attribute->designator;
        }
        return ret;
    }
    case ast::name_kind_e::CALL: {
        auto prefix = to_string(n->prefix);
        std::string ret = prefix;
        ret.push_back('(');
        if(n->arguments) {
            for(std::size_t i = 0; i < n->arguments->associations.size(); ++i) {
                if(i)
                    ret += ", ";
                ret += to_string(n->arguments->associations[i]);
            }
        }
        ret.push_back(')');
        return ret;
    }
    }
    return {};
}
} // namespace detail

inline std::string to_string(const ast::primary_item r) { return detail::to_string(r); }

inline std::string to_string(const ast::expression_item r) { return detail::to_string(r); }

inline std::string to_string(ast::explicit_range const* r) { return detail::to_string(r); }

inline std::string to_string(ast::name_node const* n) { return detail::to_string(n); }
} // namespace ast
