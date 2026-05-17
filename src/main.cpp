#include "ast_printer.h"
#include "elaborator.h"
#include <array>
#include <ast_nodes.h>
#include <filesystem>
#include <iostream>
#include <parser.h>
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
    auto syntax_error_count = 0u;
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
                syntax_error_count++;
            }
        }
    vhdl_fe::elaborator elab(parser);
    elab.add_design_files(results);
    elab.resolve_references();
    std::array<std::string, 2> severity_str{"WARN", "ERR"};
    for(auto& diag : elab.get_diagnostics()) {
        std::cerr << "[" << severity_str[static_cast<unsigned>(diag.severity)] << "]: " << diag.message << "\n";
    }
    if(enable_print_ast) {
        for(auto& df : results)
            print_ast(std::cout, df);
        std::cout << std::endl;
    }
    return syntax_error_count;
}
