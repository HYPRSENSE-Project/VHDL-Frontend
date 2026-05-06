#include "not_implemented_logger.h"
#include <antlr4-runtime.h>

namespace parser {

bool NotImplementedLogger::ENABLE = true;

void NotImplementedLogger::print(const char* msg, antlr4::ParserRuleContext* ctx) {
    if(NotImplementedLogger::ENABLE) {
        auto t = ctx->getStart();
        auto is = t->getInputStream();
        auto file = is->getSourceName();
        auto line = t->getLine() - 1; // 1..n
        auto ch = t->getCharPositionInLine();
        std::cerr << file << ":" << line << ":" << ch << ": " << msg << " Parsing of rule for '..." << ctx->getText()
                  << "...' not implemented" << std::endl;
    }
}

void NotImplementedLogger::print(const std::string& msg, antlr4::ParserRuleContext* ctx) { NotImplementedLogger::print(msg.c_str(), ctx); }
void NotImplementedLogger::print(const std::string& msg, antlr4::tree::TerminalNode* ctx) {
    if(NotImplementedLogger::ENABLE) {
        auto t = ctx->getSymbol();
        auto is = t->getInputStream();
        auto file = is->getSourceName();
        auto line = t->getLine() - 1; // 1..n
        auto ch = t->getCharPositionInLine();
        std::cerr << file << ":" << line << ":" << ch << ": " << msg << " Parsing of terminal '..." << ctx->getText()
                  << "...' not implemented" << std::endl;
    }
}
} // namespace parser
