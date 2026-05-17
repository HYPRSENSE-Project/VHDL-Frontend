[![CMake Build  & Test](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml)

# VHDL-Frontend

The current state uses the grammar from [hdlConvertor](https://github.com/Nic30/hdlConvertor.git).
The parser is a complete reimplementation with the goal to create an elaborated versions of the primary unit(s).
The AST can be found [src/ast/ast_nodes.h](https://github.com/HYPRSENSE-Project/VHDL-Frontend/blob/feature/standalone_parser/src/ast/ast_nodes.h).
It holds the parsed elements as well as the resolved references after elaboration.

The grammar is missing the following keywords which are reserved in VHDL2008:

 * assume
 * assume_guarantee
 * cover
 * default (wrong syntax)
 * fairness
 * restrict
 * restrict_guarantee
 * strong
 * vmode
 * vprop
