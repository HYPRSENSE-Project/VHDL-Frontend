#pragma once

#include <ast/ast_nodes.h>
#include <iosfwd>

void print_ast(std::ostream& os, ast::design_file* top);
