// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include "ast_nodes.h"
#include <memory>
#include <type_traits>
#include <vector>

namespace antlr4 {
class ParserRuleContext;
}
namespace ast {

void init_source_loc(source_loc*, antlr4::ParserRuleContext const* source);

struct ast_node_factory {
    template <typename T> T* create(antlr4::ParserRuleContext const* ctx = nullptr) {
        auto n = new T();
        if constexpr(std::is_base_of_v<source_loc, T>)
            init_source_loc(n, ctx);
        registry.emplace_back(n, [](void* p) { delete static_cast<T*>(p); });
        return n;
    }

private:
    std::vector<std::unique_ptr<void, void (*)(void*)>> registry;
};
} // namespace ast
