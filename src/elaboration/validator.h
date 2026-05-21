// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include "elaborator.h"
#include <ast_nodes.h>
#include <vector>

namespace vhdl_fe {

std::vector<elaboration_diagnostic> validate_resolved_ast(const std::vector<ast::design_file*>& design_files);

} // namespace vhdl_fe
