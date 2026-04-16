#include "ast_printer.h"

#include <filesystem>
#include <hdlConvertor/hdlAst/hdlContext.h>
#include <hdlConvertor/hdlConvertor.h>
#include <iostream>

#include <hdlConvertor/vhdlConvertor/designFileParser.h>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlLexer.h>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlParser.h>

class VHDLParserContainer : public hdlConvertor::iParserContainer<
                                vhdl_antlr::vhdlLexer, vhdl_antlr::vhdlParser,
                                hdlConvertor::vhdl::VhdlDesignFileParser> {
  using iParserContainer::iParserContainer;
  virtual void parseFn() override {
    vhdl_antlr::vhdlParser::Design_fileContext *tree =
        antlrParser->design_file();
    syntaxErrLogger.check_errors(); // Throw exception if errors
    hdlParser->visitDesign_file(tree);
  }
};

bool parse_vhdl(hdlConvertor::hdlAst::HdlContext &ctx,
                const std::string &path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("VHDL source not found: " + path);
  }
  std::cout << "Parsing file " << path << std::endl;
  try {
    hdlConvertor::verilog_pp::MacroDB defineDB;
    VHDLParserContainer pc(ctx, hdlConvertor::Language::VHDL, defineDB);
    pc.parse_file(path, "utf-8", false);
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return false;
  }
  return true;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: vhdl_fe <file.vhd> [<file.vhd>]*\n";
    return 1;
  }
  int ret = 0;
  try {
    hdlConvertor::hdlAst::HdlContext ctx;
    for (auto i = 1; i < argc; i++)
      ret += static_cast<int>(!parse_vhdl(ctx, argv[i]));
    // print_ast(std::cout, ctx);
    // std::cout << std::endl;
    return ret;
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return -1;
  }
}
