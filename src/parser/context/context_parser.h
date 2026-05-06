#include <ast/ast_node_factory.h>
#include <hdlConvertor/vhdlConvertor/vhdlParser/vhdlParser.h>
namespace parser {
namespace context {
ast::design_file* parse(vhdl_antlr::vhdlParser::Design_fileContext*, ast::ast_node_factory&);
ast::design_unit* parse(vhdl_antlr::vhdlParser::Design_unitContext*, ast::ast_node_factory&);
ast::library_clause* parse(vhdl_antlr::vhdlParser::Library_clauseContext*, ast::ast_node_factory&);
ast::use_clause* parse(vhdl_antlr::vhdlParser::Use_clauseContext*, ast::ast_node_factory&);
ast::context_reference* parse(vhdl_antlr::vhdlParser::Context_referenceContext*, ast::ast_node_factory&);
std::vector<ast::interface_declaration*> parse(vhdl_antlr::vhdlParser::Interface_elementContext*, ast::ast_node_factory&);
std::vector<ast::interface_declaration*> parse(vhdl_antlr::vhdlParser::Interface_signal_declarationContext*, ast::ast_node_factory&);
ast::entity_declaration* parse(vhdl_antlr::vhdlParser::Entity_declarationContext*, ast::ast_node_factory&);
ast::configuration_declaration* parse(vhdl_antlr::vhdlParser::Configuration_declarationContext*, ast::ast_node_factory&);
ast::package_declaration* parse(vhdl_antlr::vhdlParser::Package_declarationContext*, ast::ast_node_factory&);
ast::package_instantiation_declaration* parse(vhdl_antlr::vhdlParser::Package_instantiation_declarationContext*, ast::ast_node_factory&);
ast::context_declaration* parse(vhdl_antlr::vhdlParser::Context_declarationContext*, ast::ast_node_factory&);
ast::architecture_body* parse(vhdl_antlr::vhdlParser::Architecture_bodyContext*, ast::ast_node_factory&);
ast::package_body* parse(vhdl_antlr::vhdlParser::Package_bodyContext*, ast::ast_node_factory&);
} // namespace context
} // namespace parser