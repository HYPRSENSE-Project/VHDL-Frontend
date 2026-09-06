// SPDX-License-Identifier: MIT
// opyright (c) 2026 MINRES Technologies GmbH

#pragma once

#include "ast_node_factory.h"
#include "ast_nodes.h"
#include "context_parser.h"
#include "encoding_conversions.h"
#include "syntax_error_logger.h"
#include <antlr4-runtime.h>
#include <array>
#include <embedded_vhdl.h>
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
        std::vector<ast::design_file*> files = create_std_packages();
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

    std::vector<ast::design_file*> create_std_packages() {
        auto df_standard = anf.create<ast::design_file>();
        df_standard->lib_name = "STD";
        auto pd_standard = anf.create<ast::package_declaration>();
        pd_standard->my_file = df_standard;
        pd_standard->identifier = "STANDARD";
        for(auto i : {
                "DIRECTION",
                "BOOLEAN",
                "BIT",
                "CHARACTER",
                "SEVERITY_LEVEL",
                "INTEGER",
                "REAL",
                "TIME",
                "STRING",
                "BOOLEAN_VECTOR",
                "BIT_VECTOR",
                "INTEGER_VECTOR",
                "REAL_VECTOR",
                "TIME_VECTOR",
                "FILE_OPEN_KIND",
                "FILE_OPEN_STATUS",
                "FILE_OPEN_STATE",
                "FILE_ORIGIN_KIND",
            }) {
            auto type_decl = anf.create<ast::type_declaration>();
            type_decl->identifier = i;
            pd_standard->declarations.push_back(type_decl);
        }
        for(auto i : {"DELAY_LENGTH", "NATURAL", "POSITIVE"}) {
            auto type_decl = anf.create<ast::subtype_declaration>();
            type_decl->identifier = i;
            pd_standard->declarations.push_back(type_decl);
        }
        auto now_decl = anf.create<ast::subprogram_declaration>();
        now_decl->designator = "NOW";
        pd_standard->declarations.push_back(now_decl);
        auto foreign_decl = anf.create<ast::attribute_declaration>();
        foreign_decl->identifier = "FOREIGN";
        foreign_decl->type = anf.create<ast::type_mark>();
        foreign_decl->type->name = anf.create<ast::name_node>();
        foreign_decl->type->name->value = "STRING";
        pd_standard->declarations.push_back(foreign_decl);
        df_standard->units.push_back(pd_standard);
        auto df_textio = anf.create<ast::design_file>();
        df_textio->lib_name = "STD";
        auto pd_textio = anf.create<ast::package_declaration>();
        pd_textio->my_file = df_textio;
        pd_textio->identifier = "TEXTIO";
        for(auto i : {"LINE", "LINE_VECTOR", "TEXT", "SIDE"}) {
            auto type_decl = anf.create<ast::type_declaration>();
            type_decl->identifier = i;
            pd_textio->declarations.push_back(type_decl);
        }
        auto width_decl = anf.create<ast::subtype_declaration>();
        width_decl->identifier = "WIDTH";
        pd_textio->declarations.push_back(width_decl);
        //   function JUSTIFY (VALUE: STRING; JUSTIFIED: SIDE := RIGHT; FIELD: WIDTH := 0 ) return STRING;
        auto funct_decl = anf.create<ast::subprogram_declaration>();
        funct_decl->designator = "JUSTIFY";
        funct_decl->is_function = true;
        auto justify_return_type = anf.create<ast::type_mark>();
        justify_return_type->name = anf.create<ast::name_node>();
        justify_return_type->name->value = "STRING";
        funct_decl->return_type = justify_return_type;
        pd_textio->declarations.push_back(funct_decl);
        for(auto i : {"INPUT", "OUTPUT"}) {
            auto file_decl = anf.create<ast::file_declaration>();
            file_decl->identifier = i;
            pd_textio->declarations.push_back(file_decl);
        }
        for(auto i : {"READLINE", "OREAD", "HREAD", "WRITELINE", "TEE", "WRITE", "OWRITE", "HWRITE"}) {
            auto file_decl = anf.create<ast::subprogram_declaration>();
            file_decl->designator = i;
            pd_textio->declarations.push_back(file_decl);
        }
        std::array<std::tuple<std::string, std::string>, 11> aliases{
            std::make_tuple("STRING_READ", "SREAD"),
            {"BREAD", "READ"},
            {"BINARY_READ", "READ"},
            {"OCTAL_READ", "OREAD"},
            {"HEX_READ", "HREAD"},
            {"SWRITE", "WRITE"},
            {"STRING_WRITE", "WRITE"},
            {"BWRITE", "WRITE"},
            {"BINARY_WRITE", "WRITE"},
            {"OCTAL_WRITE", "OWRITE"},
            {"HEX_WRITE", "HWRITE"},
        };
        for(auto i : aliases) {
            auto alias_decl = anf.create<ast::alias_declaration>();
            alias_decl->alias_designator = std::get<0>(i);
            alias_decl->name = std::get<1>(i);
            pd_textio->declarations.push_back(alias_decl);
        }
        df_textio->units.push_back(pd_textio);
        return {df_standard, df_textio};
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
