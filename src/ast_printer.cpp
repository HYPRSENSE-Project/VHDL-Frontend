#include "ast_printer.h"

#include <hdlConvertor/hdlAst/hdlCompInst.h>
#include <hdlConvertor/hdlAst/hdlContext.h>
#include <hdlConvertor/hdlAst/hdlIdDef.h>
#include <hdlConvertor/hdlAst/hdlLibrary.h>
#include <hdlConvertor/hdlAst/hdlModuleDec.h>
#include <hdlConvertor/hdlAst/hdlModuleDef.h>
#include <hdlConvertor/hdlAst/hdlNamespace.h>
#include <hdlConvertor/hdlAst/hdlOp.h>
#include <hdlConvertor/hdlAst/hdlStmAssign.h>
#include <hdlConvertor/hdlAst/hdlStmBlock.h>
#include <hdlConvertor/hdlAst/hdlStmExpr.h>
#include <hdlConvertor/hdlAst/hdlStmIf.h>
#include <hdlConvertor/hdlAst/hdlStmProcess.h>
#include <hdlConvertor/hdlAst/hdlStm_others.h>
#include <hdlConvertor/hdlAst/hdlValue.h>
#include <hdlConvertor/hdlAst/iHdlExpr.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace {

using namespace hdlConvertor::hdlAst;

void indent(std::ostream &os, int level) {
  for (int i = 0; i < level; ++i) {
    os.put(' ');
  }
}

void print_expr(std::ostream &os, const iHdlExprItem *expr);
void print_obj(std::ostream &os, const iHdlObj *obj, int level);

bool is_ident_char(char c) {
  const auto uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) || c == '_';
}

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

char lower_char(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

std::string trim(std::string_view text) {
  size_t start = 0;
  size_t end = text.size();
  while (start < end && is_space(text[start])) {
    ++start;
  }
  while (end > start && is_space(text[end - 1])) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

void skip_space(std::string_view text, size_t &pos) {
  while (pos < text.size() && is_space(text[pos])) {
    ++pos;
  }
}

bool match_keyword(std::string_view text, size_t pos,
                   std::string_view keyword) {
  if (pos + keyword.size() > text.size()) {
    return false;
  }
  if (pos > 0 && is_ident_char(text[pos - 1])) {
    return false;
  }
  for (size_t i = 0; i < keyword.size(); ++i) {
    if (lower_char(text[pos + i]) != keyword[i]) {
      return false;
    }
  }
  const size_t end = pos + keyword.size();
  return end >= text.size() || !is_ident_char(text[end]);
}

std::string parse_identifier(std::string_view text, size_t &pos) {
  skip_space(text, pos);
  const size_t start = pos;
  while (pos < text.size() && is_ident_char(text[pos])) {
    ++pos;
  }
  return std::string(text.substr(start, pos - start));
}

std::string strip_vhdl_comments(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  bool in_string = false;
  for (size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (!in_string && c == '-' && i + 1 < text.size() && text[i + 1] == '-') {
      while (i < text.size() && text[i] != '\n') {
        out.push_back(' ');
        ++i;
      }
      if (i < text.size()) {
        out.push_back(text[i]);
      }
      continue;
    }
    if (c == '"') {
      in_string = !in_string;
    }
    out.push_back(c);
  }
  return out;
}

std::string read_file(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Unable to open VHDL source: " + path);
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

void print_expr_list(std::ostream &os,
                     const std::vector<std::unique_ptr<iHdlExprItem>> &exprs) {
  bool first = true;
  for (const auto &expr : exprs) {
    if (!first) {
      os << ", ";
    }
    first = false;
    print_expr(os, expr.get());
  }
}

void print_path(std::ostream &os,
                const std::vector<std::unique_ptr<iHdlExprItem>> &path) {
  bool first = true;
  for (const auto &item : path) {
    if (!first) {
      os << '.';
    }
    first = false;
    print_expr(os, item.get());
  }
}

void print_expr(std::ostream &os, const iHdlExprItem *expr) {
  if (!expr) {
    os << "<null>";
    return;
  }

  if (const auto *id = dynamic_cast<const HdlValueId *>(expr)) {
    os << id->_str;
    return;
  }
  if (const auto *value = dynamic_cast<const HdlValueInt *>(expr)) {
    if (value->_int.is_bitstring()) {
      os << value->_int.bitstring;
    } else {
      os << value->_int.val;
    }
    return;
  }
  if (const auto *value = dynamic_cast<const HdlValueStr *>(expr)) {
    os << '"' << value->_str << '"';
    return;
  }
  if (const auto *value = dynamic_cast<const HdlValueFloat *>(expr)) {
    os << value->_float;
    return;
  }
  if (const auto *value = dynamic_cast<const HdlValueSymbol *>(expr)) {
    os << HdlValueSymbol::toString(value->symb);
    return;
  }
  if (const auto *value = dynamic_cast<const HdlValueArr *>(expr)) {
    os << '[';
    if (value->_arr) {
      print_expr_list(os, *value->_arr);
    }
    os << ']';
    return;
  }
  if (const auto *op = dynamic_cast<const HdlOp *>(expr)) {
    os << HdlOpType_toString(op->op) << '(';
    print_expr_list(os, op->operands);
    os << ')';
    return;
  }

  os << "<expr:" << typeid(*expr).name() << '>';
}

void print_id_def(std::ostream &os, const HdlIdDef &id, int level) {
  indent(os, level);
  if (id.direction != DIR_INTERNAL) {
    os << "Port " << HdlDirection_toString(id.direction) << ' ';
  } else if (id.is_const) {
    os << "Const ";
  } else if (id.is_latched) {
    os << "Var ";
  } else {
    os << "Signal ";
  }

  os << id.name;
  if (id.type) {
    os << " : ";
    print_expr(os, id.type.get());
  }
  if (id.value) {
    os << " := ";
    print_expr(os, id.value.get());
  }
  os << '\n';
}

void print_obj(std::ostream &os, const iHdlObj *obj, int level) {
  if (!obj) {
    indent(os, level);
    os << "<null>\n";
    return;
  }

  if (const auto *module = dynamic_cast<const HdlModuleDec *>(obj)) {
    indent(os, level);
    os << "Entity " << module->name << '\n';
    for (const auto &generic : module->generics) {
      print_id_def(os, *generic, level + 2);
    }
    for (const auto &port : module->ports) {
      print_id_def(os, *port, level + 2);
    }
    for (const auto &child : module->objs) {
      print_obj(os, child.get(), level + 2);
    }
    return;
  }

  if (const auto *library = dynamic_cast<const HdlLibrary *>(obj)) {
    indent(os, level);
    os << "Library " << library->name << '\n';
    return;
  }

  if (const auto *module = dynamic_cast<const HdlModuleDef *>(obj)) {
    indent(os, level);
    os << "Architecture " << module->name;
    if (module->module_name) {
      os << " of ";
      print_expr(os, module->module_name.get());
    }
    os << '\n';
    for (const auto &child : module->objs) {
      print_obj(os, child.get(), level + 2);
    }
    return;
  }

  if (const auto *id = dynamic_cast<const HdlIdDef *>(obj)) {
    print_id_def(os, *id, level);
    return;
  }

  if (const auto *process = dynamic_cast<const HdlStmProcess *>(obj)) {
    indent(os, level);
    os << "Process";
    if (process->sensitivity_list && !process->sensitivity_list->empty()) {
      os << " (";
      print_expr_list(os, *process->sensitivity_list);
      os << ')';
    }
    os << '\n';
    print_obj(os, process->body.get(), level + 2);
    return;
  }

  if (const auto *import = dynamic_cast<const HdlStmImport *>(obj)) {
    indent(os, level);
    os << "Use ";
    print_path(os, import->path);
    os << '\n';
    return;
  }

  if (const auto *block = dynamic_cast<const HdlStmBlock *>(obj)) {
    indent(os, level);
    os << "Block " << HdlStmBlockJoinType_toString(block->join_t) << '\n';
    for (const auto &statement : block->statements) {
      print_obj(os, statement.get(), level + 2);
    }
    return;
  }

  if (const auto *assign = dynamic_cast<const HdlStmAssign *>(obj)) {
    indent(os, level);
    os << "Assign ";
    print_expr(os, assign->dst.get());
    os << (assign->is_blocking ? " := " : " <= ");
    print_expr(os, assign->src.get());
    os << '\n';
    return;
  }

  if (const auto *expr_stm = dynamic_cast<const HdlStmExpr *>(obj)) {
    indent(os, level);
    os << "Expr ";
    print_expr(os, expr_stm->expr.get());
    os << '\n';
    return;
  }

  if (const auto *if_stm = dynamic_cast<const HdlStmIf *>(obj)) {
    indent(os, level);
    os << "If ";
    print_expr(os, if_stm->cond.get());
    os << '\n';
    print_obj(os, if_stm->ifTrue.get(), level + 2);
    for (const auto &else_if : if_stm->elseIfs) {
      indent(os, level);
      os << "ElseIf ";
      print_expr(os, else_if.expr.get());
      os << '\n';
      print_obj(os, else_if.obj.get(), level + 2);
    }
    if (if_stm->ifFalse) {
      indent(os, level);
      os << "Else\n";
      print_obj(os, if_stm->ifFalse.get(), level + 2);
    }
    return;
  }

  if (const auto *wait = dynamic_cast<const HdlStmWait *>(obj)) {
    indent(os, level);
    os << "Wait";
    if (!wait->val.empty()) {
      os << ' ';
      print_expr_list(os, wait->val);
    }
    os << '\n';
    return;
  }

  if (const auto *instance = dynamic_cast<const HdlCompInst *>(obj)) {
    indent(os, level);
    os << "Instance ";
    print_expr(os, instance->name.get());
    os << " : ";
    print_expr(os, instance->module_name.get());
    os << '\n';
    return;
  }

  if (const auto *space = dynamic_cast<const HdlValueIdspace *>(obj)) {
    indent(os, level);
    os << "Namespace " << space->name << '\n';
    for (const auto &child : space->objs) {
      print_obj(os, child.get(), level + 2);
    }
    return;
  }

  if (const auto *statement = dynamic_cast<const iHdlStatement *>(obj)) {
    indent(os, level);
    os << "Statement " << typeid(*statement).name() << '\n';
    return;
  }

  indent(os, level);
  os << "Object " << typeid(*obj).name() << '\n';
}

} // namespace

void print_ast(std::ostream &os, const hdlConvertor::hdlAst::HdlContext &ctx) {
  for (const auto &obj : ctx.objs) {
    print_obj(os, obj.get(), 0);
  }
}
