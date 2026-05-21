// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include <ast_helper.h>
#include <ast_nodes.h>
#include <iosfwd>

void print_ast(std::ostream& os, ast::design_file* top);
