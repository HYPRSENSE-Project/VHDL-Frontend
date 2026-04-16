#pragma once

#include <iosfwd>
#include <string>

namespace hdlConvertor::hdlAst {
class HdlContext;
}

void print_ast(std::ostream &os, const hdlConvertor::hdlAst::HdlContext &ctx);
