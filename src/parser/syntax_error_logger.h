#pragma once

#include <sstream>
#include <string>
#include <vector>

#include <antlr4-runtime.h>
namespace parser {

class ErrorData {
public:
    enum error_kind_t { SYNTAXERROR, REPORTAMBIGUITY, REPORTCONTEXTSENSITIVITY, REPORTATTEMPTINGFULLCONTEXT };
    enum error_kind_t error_kind;
    size_t line;
    size_t charPosition;
    std::string filename;
    std::string message;
};

/*
 * The class which implements ANTLR error listener which is installed in parser
 * and lexer and staging the errors for later check.
 * */
class SyntaxErrorLogger : public antlr4::ANTLRErrorListener {

private:
    std::vector<ErrorData> _errors;

public:
    std::string error_prefix;

    SyntaxErrorLogger();

    void check_errors();
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr e);

    void reportAmbiguity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex, bool exact,
                         const antlrcpp::BitSet& ambigAlts, antlr4::atn::ATNConfigSet* configs);

    void reportContextSensitivity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
                                  size_t prediction, antlr4::atn::ATNConfigSet* configs);

    void reportAttemptingFullContext(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
                                     const antlrcpp::BitSet& conflictingAlts, antlr4::atn::ATNConfigSet* configs);
};
} // namespace parser
