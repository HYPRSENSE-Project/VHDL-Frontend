// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include <cstddef>
#include <string>

namespace ast {
struct error_data {
    enum error_kind_t {
        SYNTAXERROR,
        REPORTAMBIGUITY,
        REPORTCONTEXTSENSITIVITY,
        REPORTATTEMPTINGFULLCONTEXT,
        ELABORATIONWARNING,
        ELABORATIONERROR,
        VALIDATIONERROR
    };
    enum error_kind_t error_kind;
    std::string message;
    std::string filename;
    size_t line;
    size_t charPosition;
};
} // namespace ast