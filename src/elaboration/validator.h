// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include <ast_nodes.h>
#include <error_data.h>
#include <vector>

namespace vhdl_fe {

std::vector<ast::error_data> validate_resolved_ast(const std::vector<ast::design_file*>& design_files);

} // namespace vhdl_fe
