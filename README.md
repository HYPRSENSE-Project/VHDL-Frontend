[![CMake Build  & Test](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml)

# VHDL-Frontend

The current state uses the parser from [hdlConvertor](https://github.com/Nic30/hdlConvertor.git) as engine. Using the (Zamiacad)[git://git.code.sf.net/p/zamiacad/code] examples and (GHDL)[https://github.com/ghdl/ghdl.git] testsuite indicate, that the following AST elements are not yet supported:
 * VhdlStatementParser.visitCase_statement
 * VhdlStatementParser.visitConcurrent_procedure_call_statement
 * VhdlStatementParser.visitConcurrent_selected_signal_assignment
 * VhdlStatementParser.visitConcurrent_signal_assignment_statement
 * VhdlStatementParser.visitConditional_variable_assignment
 * VhdlStatementParser.visitSelected_signal_assignment
 * VhdlStatementParser.visitSelected_variable_assignments
 * VhdlStatementParser.visitSimple_waveform_assignment

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
