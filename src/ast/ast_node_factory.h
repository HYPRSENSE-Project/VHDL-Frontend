#pragma once
#include "ast_nodes.h"
#include <any>
#include <memory>
#include <vector>
namespace ast {
struct ast_node_factory {
    template <typename T> T* create() {
        auto n = new T();
        registry.emplace_back(n, [](void* p) { delete static_cast<T*>(p); });
        return n;
    }

private:
    std::vector<std::unique_ptr<void, void (*)(void*)>> registry;
};
} // namespace ast
