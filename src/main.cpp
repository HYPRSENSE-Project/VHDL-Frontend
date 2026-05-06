#include "ast_printer.h"
#include "parser/parser.h"
#include <filesystem>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlLexer.h>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlParser.h>
#include <iostream>
#include <parser/parser.h>

bool parse_vhdl(const std::filesystem::path& path) {
    if(!std::filesystem::exists(path)) {
        throw std::runtime_error(std::string("VHDL source not found: ") + path.c_str());
    }
    std::cout << "Parsing file " << path << std::endl;
    try {
#if 0
        VHDLParserContainer pc(ctx);
        pc.parse_file(path, "utf-8", false);
#else
        parser::Parser parser;
        parser.parse_file(path, parser::encoding::UTF_8, true);
#endif
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: vhdl_fe <file.vhd> [<file.vhd>]*\n";
        return 1;
    }
    int ret = 0;
    bool enable_print_ast = false;
    try {
        for(auto i = 1; i < argc; i++)
            if(strncmp(argv[i], "-v", 2) == 0)
                enable_print_ast = true;
            else {
                const std::filesystem::path path = argv[i];
                ret += static_cast<int>(!parse_vhdl(path));
                // if(enable_print_ast) {
                //     print_ast(std::cout, ctx);
                //     std::cout << std::endl;
                // }
            }
        return ret;
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return -1;
    }
}
