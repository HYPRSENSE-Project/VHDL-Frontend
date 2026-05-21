// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH
// Copyright (c) 2015 Nic30

#pragma once

#include <error_data.h>
#include <string>
#include <vector>

#include <antlr4-runtime.h>

namespace parser {

class parse_exception : public std::exception {
private:
    std::string _msg;

public:
    parse_exception(std::string msg) throw()
    : _msg(msg) {}
    virtual ~parse_exception() = default;
    virtual const char* what() const throw() { return _msg.c_str(); }
};

/*
 * The class which implements ANTLR error listener which is installed in parser
 * and lexer and staging the errors for later check.
 * */
class syntax_error_logger : public antlr4::ANTLRErrorListener {

private:
    std::vector<ast::error_data> _errors;

public:
    std::string error_prefix;

    syntax_error_logger();

    void check_errors();
    void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line, size_t charPositionInLine,
                     const std::string& msg, std::exception_ptr e) override;

    void reportAmbiguity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex, bool exact,
                         const antlrcpp::BitSet& ambigAlts, antlr4::atn::ATNConfigSet* configs) override;

    void reportContextSensitivity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
                                  size_t prediction, antlr4::atn::ATNConfigSet* configs) override;

    void reportAttemptingFullContext(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
                                     const antlrcpp::BitSet& conflictingAlts, antlr4::atn::ATNConfigSet* configs) override;
};
} // namespace parser
