#pragma once

#include "ast_node_factory.h"
#include "ast_nodes.h"
#include "context_parser.h"
#include "encoding_conversions.h"
#include "syntax_error_logger.h"
#include <antlr4-runtime.h>
#include <fstream>
#include <iostream>
#include <vhdlParser/vhdlLexer.h>
#include <vhdlParser/vhdlParser.h>

namespace parser {
using namespace vhdl_antlr;
struct Parser {
    SyntaxErrorLogger syntaxErrLogger;
    ast::ast_node_factory anf;

    /*
     * :param context: if context is nullptr new context is generated
     *                 otherwise specified context is used
     * */
    ast::design_file* parse_file(const std::filesystem::path& file_name, encoding enc, std::string lib_name) {
        auto input_stream = ANTLRFileStream_with_encoding(file_name, enc);
        input_stream.name = file_name.u8string();
        return _parse(input_stream, lib_name);
    }

    ast::design_file* parse_str(const std::string& input_str, encoding enc, std::string lib_name) {
        antlr4::ANTLRInputStream input_stream(_to_utf8(input_str, enc));
        input_stream.name = "<string>";
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
