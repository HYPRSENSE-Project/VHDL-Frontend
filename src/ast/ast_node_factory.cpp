#include "ast_node_factory.h"
#include "antlr4-runtime.h"
#include <ParserRuleContext.h>

namespace ast {
namespace {
std::string filenameOf(const antlr4::Token* tok) {
    if(tok == nullptr)
        return "<no token>";
    auto* input = tok->getInputStream();
    if(input != nullptr)
        return input->getSourceName();
    auto* source = tok->getTokenSource();
    if(source != nullptr)
        return source->getSourceName();
    return "<unknown source>";
}
} // namespace

void init_source_loc(source_loc* n, antlr4::ParserRuleContext const* ctx) {
    if(!n || !ctx)
        return;
    auto tok = ctx->getStart();
    n->file = filenameOf(tok);
    n->line = tok->getLine();
    n->col = tok->getCharPositionInLine() + 1;
}
} // namespace ast