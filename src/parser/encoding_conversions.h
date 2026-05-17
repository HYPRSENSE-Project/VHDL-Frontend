// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH
// Copyright (c) 2015 Nic30

#pragma once

#include <ANTLRInputStream.h>
#include <filesystem>
#include <string>

namespace parser {
enum class encoding { UTF_8, UTF_16, ISO_8859_1 };

std::string iso_8859_1_to_utf8(const std::string& str);
std::string _to_utf8(const std::string& str, encoding enc);
antlr4::ANTLRInputStream antlr_file_stream_with_encoding(const std::filesystem::path& file_name, encoding enc);

} // namespace parser
