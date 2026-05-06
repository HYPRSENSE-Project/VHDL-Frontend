#pragma once

#include "ast/ast_node_factory.h"
#include "context/context_parser.h"
#include "encodingConversions.h"
#include "syntaxErrorLogger.h"
#include <antlr4-runtime.h>
#include <fstream>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlLexer.h>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlParser.h>
#include <iostream>

namespace parser {
using namespace vhdl_antlr;
class Parser {
    SyntaxErrorLogger syntaxErrLogger;

public:
    /*
     * :param context: if context is nullptr new context is generated
     *                 otherwise specified context is used
     * */
    void parse_file(const std::filesystem::path& file_name, encoding enc, bool hierarchyOnly) {
        std::ifstream ifs(file_name);
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        std::string str(size, ' ');
        ifs.seekg(0);
        ifs.read(&str[0], size);
        str = _to_utf8(str, enc);

        auto input_stream = ANTLRFileStream_with_encoding(file_name, enc);
        input_stream.name = file_name.u8string();
        _parse(input_stream, hierarchyOnly);
    }

    void parse_str(const std::string& input_str, encoding enc, bool hierarchyOnly) {
        antlr4::ANTLRInputStream input_stream(_to_utf8(input_str, enc));
        input_stream.name = "<string>";
        _parse(input_stream, hierarchyOnly);
    }

    void _parse(antlr4::ANTLRInputStream& input_stream, bool hierarchyOnly) {
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
            ast::ast_node_factory anf;
            context::parse(tree, anf);
        } catch(const antlr4::NoViableAltException& e) {
            // [todo] check if error really appeared in syntaxErrLogger
            throw;
        }
        syntaxErrLogger.error_prefix = "";
        syntaxErrLogger.check_errors(); // Throw exception if errors
    }
};
} // namespace parser
