// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#include "ast_nodes.h"
#include "ast_node_factory.h"
#include <type_traits>
#include <variant>

namespace ast {

component_instantiation_statement* component_instantiation_statement::clone(ast_node_factory& anf) {
    auto ret = anf.create<component_instantiation_statement>();
    *ret = *this;
    return ret;
}

architecture_body* architecture_body::clone(ast_node_factory& anf) {
    auto ret = anf.create<architecture_body>();
    *ret = *this;
    ret->concurrent_statements.clear();
    for(auto* stmt : concurrent_statements) {
        if(!stmt) {
            ret->concurrent_statements.push_back(nullptr);
            continue;
        }
        auto* stmt_copy = anf.create<concurrent_statement>();
        *stmt_copy = *stmt;
        std::visit(
            [&anf, stmt_copy](auto* node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr(std::is_same_v<T, component_instantiation_statement*>) {
                    stmt_copy->statement = node ? node->clone(anf) : nullptr;
                } else {
                    stmt_copy->statement = node;
                }
            },
            stmt->statement);
        ret->concurrent_statements.push_back(stmt_copy);
    }
    return ret;
}

} // namespace ast
