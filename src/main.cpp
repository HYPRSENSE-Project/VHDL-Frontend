#include "ast/ast_nodes.h"
#include "ast_printer.h"
#include "parser/parser.h"
#include <filesystem>
#include <iostream>
#include <parser/parser.h>
#include <vector>
#include <vhdlParser/vhdlLexer.h>
#include <vhdlParser/vhdlParser.h>

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: vhdl_fe <file.vhd> [<file.vhd>]*\n";
        return 1;
    }
    bool enable_print_ast = false;
    parser::Parser parser;
    std::vector<ast::design_file*> results;
    std::string actual_libname = "work";
    for(auto i = 1; i < argc; i++)
        if(strncmp(argv[i], "-v", 2) == 0)
            enable_print_ast = true;
        else if(strncmp(argv[i], "-l", 2) == 0) {
            if(++i >= argc)
                throw std::runtime_error("not enough arguments to '-l' switch");
            actual_libname = argv[i];
        } else {
            try {
                const std::filesystem::path path = argv[i];
                if(!std::filesystem::exists(path)) {
                    throw std::runtime_error(std::string("VHDL source not found: ") + argv[i]);
                }
                std::cout << "Parsing file " << path << std::endl;
                results.push_back(parser.parse_file(path, parser::encoding::UTF_8, actual_libname));
            } catch(const std::exception& e) {
                std::cerr << "error: " << e.what() << "\n";
                std::cerr << "file will be exlecuded from elaboration";
            }
        }
    if(enable_print_ast) {
        for(auto& df : results)
            print_ast(std::cout, df);
        std::cout << std::endl;
    }
    return 0;
}
