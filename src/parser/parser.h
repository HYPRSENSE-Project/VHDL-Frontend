// SPDX-License-Identifier: MIT
// opyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include "ast_node_factory.h"
#include "ast_nodes.h"
#include "context_parser.h"
#include "encoding_conversions.h"
#include "syntax_error_logger.h"
#include <antlr4-runtime.h>
#include <embedded_vhdl.h>
#include <stdexcept>
#include <string_view>
#include <vhdlParser/vhdlLexer.h>
#include <vhdlParser/vhdlParser.h>

namespace parser {
namespace {
bool ends_with(std::string_view const fullString, std::string_view const ending) {
    if(fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

} // namespace
using namespace vhdl_antlr;
struct parser {
    syntax_error_logger syntaxErrLogger;
    ast::ast_node_factory anf;
    /**
     *  Parse embedded standardized VHDL packages into design-file ASTs.
     *  Skips STANDARD, which is synthesized by the elaborator, and package bodies.
     * Resources are decoded as ISO-8859-1 and assigned to the library named by their resource path prefix.
     *
     * @return Pointers to the parsed design files.
     */
    std::vector<ast::design_file*> load_standardized_packages() {
        std::vector<ast::design_file*> files;
        const auto& resources = embedded_vhdl::all();
        files.reserve(resources.size());
        for(const auto& resource : resources) {
            // STANDARD contains implementation-defined placeholders and is synthesized by the elaborator and
            // we do not load bodies automatically as it take very long
            if(resource.name == "std/standard.vhdl" || ends_with(resource.name, "-body.vhdl"))
                continue;
            files.push_back(parse_str(std::string(resource.contents), encoding::ISO_8859_1,
                                      std::string(resource.name.substr(0, resource.name.find('/'))), std::string(resource.name)));
        }
        return files;
    }
    /**
     * @brief Parse a VHDL file into a design-file AST.
     * @param file_name Filesystem path or embedded resource name prefixed with `ieee/` or `std/`.
     * @param enc Encoding of filesystem input; embedded resources use ISO-8859-1.
     * @param lib_name Library assigned to the parsed design file; embedded resources use their prefix.
     * @return Pointer to the parsed design file.
     */
    ast::design_file* parse_file(const std::filesystem::path& file_name, encoding enc, std::string lib_name) {
        if(file_name.string().substr(0, 5) == "ieee/") {
            const auto& resource = embedded_vhdl::get(file_name.string());
            return parse_str(std::string(resource), encoding::ISO_8859_1, "ieee", file_name.string());
        }
        if(file_name.string().substr(0, 4) == "std/") {
            const auto& resource = embedded_vhdl::get(file_name.string());
            return parse_str(std::string(resource), encoding::ISO_8859_1, "std", file_name.string());
        }
        auto input_stream = antlr_file_stream_with_encoding(file_name, enc);
        return _parse(input_stream, lib_name);
    }
    /**
     * @brief Parse a VHDL string into a design-file AST.
     * @param input_str VHDL source text to parse.
     * @param enc Encoding of the source text, converted to UTF-8 before parsing.
     * @param lib_name Library assigned to the parsed design file.
     * @param source_name Source name used for diagnostics; defaults to `<string>`.
     * @return Pointer to the parsed design file.
     */
    ast::design_file* parse_str(const std::string& input_str, encoding enc, std::string lib_name, std::string source_name = "<string>") {
        antlr4::ANTLRInputStream input_stream(_to_utf8(input_str, enc));
        input_stream.name = source_name;
        return _parse(input_stream, lib_name);
    }

private:
    ast::design_file* _parse(antlr4::ANTLRInputStream& input_stream, std::string lib_name) {
        // create a lexer that feeds off of input CharStream
        vhdlLexer lexer(&input_stream);
        // create a buffer of tokens pulled from the lexer
        antlr4::CommonTokenStream tokens(&lexer);
        // create a parser that feeds off the tokens buffer
        vhdlParser antlrParser(&tokens);
        antlrParser.removeErrorListeners();
        lexer.removeErrorListeners();
        lexer.addErrorListener(&syntaxErrLogger);
        antlrParser.removeErrorListeners();
        antlrParser.addErrorListener(&syntaxErrLogger);
        try {
            vhdlParser::Design_fileContext* tree = antlrParser.design_file();
            syntaxErrLogger.check_errors(); // Throw exception if errors
            auto res = context::parse(tree, anf);
            res->lib_name = lib_name;
            syntaxErrLogger.error_prefix = "";
            syntaxErrLogger.check_errors(); // Throw exception if errors
            return res;
        } catch(const antlr4::NoViableAltException& e) {
            // [todo] check if error really appeared in syntaxErrLogger
            throw;
        }
    }
};
} // namespace parser
