#pragma once
#include "ast_nodes.h"
#include <memory>
#include <vector>
namespace ast {
struct ast_node_factory {
    template <typename T> T* create() {
        auto n = std::make_unique<T>();
        auto ret = n.get();
        registry.push_back(std::move(n));
        return ret;
    }

private:
    std::vector<std::unique_ptr<ast_node>> registry;
};
} // namespace ast