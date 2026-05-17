// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH
// Copyright (c) 2015 Nic30

#include "syntax_error_logger.h"
#include <antlr4-runtime.h>
#include <assert.h>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define UNUSED(x) x __attribute__((unused))
#elif defined(_MSC_VER)
#define UNUSED(x)
#endif

namespace parser {

using namespace std;

syntax_error_logger::syntax_error_logger()
: antlr4::ANTLRErrorListener() {}

void syntax_error_logger::check_errors() {
    stringstream error_msg;
    error_msg << endl;
    for(auto& e : _errors) {
        error_msg << "    " << e.filename << ':' << e.line << ':' << (uint32_t)e.charPosition << ": " << error_prefix
                  << " SyntaxError:" << e.message << endl;
    }
    if(_errors.size() > 0) {
        throw parse_exception(error_msg.str());
    }
}

void syntax_error_logger::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* UNUSED(offendingSymbol), size_t line,
                                      size_t charPositionInLine, const string& msg, exception_ptr UNUSED(e)) {

    error_data err;
    // use offsets and overrides from file_line_map if available
    err.line = line;
    err.filename = recognizer->getInputStream()->getSourceName();
    err.charPosition = charPositionInLine;
    err.message = msg;

    _errors.push_back(err);
    // cerr << line << ":" << charPositionInLine << ":SyntaxError:" << msg <<
    // "\n";
}

void syntax_error_logger::reportAmbiguity(antlr4::Parser* UNUSED(recognizer), const antlr4::dfa::DFA& UNUSED(dfa),
                                          size_t UNUSED(startIndex), size_t UNUSED(stopIndex), bool UNUSED(exact),
                                          const antlrcpp::BitSet& UNUSED(ambigAlts), antlr4::atn::ATNConfigSet* UNUSED(configs)) {
    // cerr << ":Ambiguity:" << std::endl;
}

void syntax_error_logger::reportContextSensitivity(antlr4::Parser* UNUSED(recognizer), const antlr4::dfa::DFA& UNUSED(dfa),
                                                   size_t UNUSED(startIndex), size_t UNUSED(stopIndex), size_t UNUSED(prediction),
                                                   antlr4::atn::ATNConfigSet* UNUSED(configs)) {
    // cerr << ":ContextSensitivity:" << std::endl;
}

void syntax_error_logger::reportAttemptingFullContext(antlr4::Parser* UNUSED(recognizer), const antlr4::dfa::DFA& UNUSED(dfa),
                                                      size_t UNUSED(startIndex), size_t UNUSED(stopIndex),
                                                      const antlrcpp::BitSet& UNUSED(conflictingAlts),
                                                      antlr4::atn::ATNConfigSet* UNUSED(configs)) {
    // cerr << ":AttemptingFullContext:" << std::endl;
}

} // namespace parser
