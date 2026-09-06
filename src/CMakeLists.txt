add_subdirectory(grammar)
add_subdirectory(resources)
add_subdirectory(ast)
add_subdirectory(parser)
add_subdirectory(elaboration)

add_executable(vhdl_fe
    main.cpp
    ast_printer.cpp
)
target_compile_definitions(vhdl_fe PRIVATE $<$<CONFIG:DEBUG>:_GLIBCXX_DEBUG>)
target_include_directories(vhdl_fe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vhdl_fe PRIVATE vhdl_fe::parser vhdl_fe::elaboration)

add_test(NAME parse_valid_001 COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/basic/001_minimal.vhd)
add_test(NAME parse_010_mux_when COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/combinational/010_mux_when.vhd)
add_test(NAME parse_011_mux_process COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/combinational/011_mux_process.vhd)
add_test(NAME parse_012_case COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/combinational/012_case.vhd)
add_test(NAME parse_020_simple_ff COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/sequential/020_simple_ff.vhd)
add_test(NAME parse_021_ff_with_reset COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/sequential/021_ff_with_reset.vhd)
add_test(NAME parse_022_pipeline COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/sequential/022_pipeline.vhd)
add_test(NAME parse_030_logic_ops COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/expressions/030_logic_ops.vhd)
add_test(NAME parse_031_arithmetic_unsigned COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/expressions/031_arithmetic_unsigned.vhd)
add_test(NAME parse_032_comparison COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/expressions/032_comparison.vhd)
add_test(NAME parse_040_simple_instance COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/valid/hierarchy/040_simple_instance.vhd)
#add_test(NAME invalid_110 COMMAND $<TARGET_FILE:vhdl_fe> ${PROJECT_SOURCE_DIR}/tests/inputs/minimal/invalid/semantic/110_unknown_signal.vhd)
#set_tests_properties(invalid_110 PROPERTIES WILL_FAIL TRUE)
