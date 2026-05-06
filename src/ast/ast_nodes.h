#pragma once
#include <string>
#include <vector>
namespace ast {
enum class signal_mode { NONE, IN, OUT, INOUT, BUFFER, LINKAGE };
struct ast_node {
    virtual ~ast_node() = default;
};
struct context_item : public ast_node {
    virtual ~context_item() = default;
};
struct library_clause : public context_item {
    std::vector<std::string> names;
    virtual ~library_clause() = default;
};

struct use_clause : public context_item {
    std::vector<std::string> names;
    virtual ~use_clause() = default;
};

struct context_reference : public context_item {
    std::vector<std::string> names;
    virtual ~context_reference() = default;
};

struct lib_unit : public ast_node {
    virtual ~lib_unit() = default;
};
struct primary_unit : public lib_unit {
    virtual ~primary_unit() = default;
};
struct interface_declaration : public ast_node {
    virtual ~interface_declaration() = default;
};

struct signal_declaration : public interface_declaration {
    std::string name;
    signal_mode mode{ast::signal_mode::NONE};
    std::string resolution;
    std::string type;
    std::string constraint;
    bool is_bus;
    virtual ~signal_declaration() = default;
};

struct entity_declaration : public primary_unit {
    std::string identifier;
    std::vector<interface_declaration*> port_list;
    virtual ~entity_declaration() = default;
};

struct configuration_declaration : public primary_unit {
    virtual ~configuration_declaration() = default;
};
struct package_declaration : public primary_unit {
    virtual ~package_declaration() = default;
};
struct package_instantiation_declaration : public primary_unit {
    virtual ~package_instantiation_declaration() = default;
};
struct context_declaration : public primary_unit {
    virtual ~context_declaration() = default;
};
struct secondary_unit : public lib_unit {
    virtual ~secondary_unit() = default;
};
struct architecture_body : public secondary_unit {
    virtual ~architecture_body() = default;
};
struct package_body : public secondary_unit {
    virtual ~package_body() = default;
};
struct design_unit : public ast_node {
    std::vector<context_item*> context_items;
    lib_unit* unit;
    virtual ~design_unit() = default;
};
struct design_file : public ast_node {
    std::vector<design_unit*> design_units;
    std::string lib_name;
    virtual ~design_file() = default;
};
} // namespace ast