cmake_minimum_required(VERSION 3.20)
project(mini_vhdl_frontend LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(ExternalProject)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/grammar)
add_subdirectory(src/ast)
add_subdirectory(src/parser)

add_executable(vhdl_fe
    #src/ast_printer.cpp
    src/main.cpp
    src/ast_printer.cpp
)

target_include_directories(vhdl_fe PRIVATE src)
target_link_libraries(vhdl_fe PRIVATE parser)

enable_testing()

add_test(NAME valid_001 COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/basic/001_minimal.vhd)
add_test(NAME 010_mux_when COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/combinational/010_mux_when.vhd)
add_test(NAME 011_mux_process COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/combinational/011_mux_process.vhd)
add_test(NAME 012_case COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/combinational/012_case.vhd)
add_test(NAME 020_simple_ff COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/sequential/020_simple_ff.vhd)
add_test(NAME 021_ff_with_reset COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/sequential/021_ff_with_reset.vhd)
add_test(NAME 022_pipeline COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/sequential/022_pipeline.vhd)
add_test(NAME 030_logic_ops COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/expressions/030_logic_ops.vhd)
add_test(NAME 031_arithmetic_unsigned COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/expressions/031_arithmetic_unsigned.vhd)
add_test(NAME 032_comparison COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/expressions/032_comparison.vhd)
add_test(NAME 040_simple_instance COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/valid/hierarchy/040_simple_instance.vhd)

#add_test(NAME invalid_110 COMMAND vhdl_fe ${CMAKE_CURRENT_LIST_DIR}/tests/minimal/invalid/semantic/110_unknown_signal.vhd)
#set_tests_properties(invalid_110 PROPERTIES WILL_FAIL TRUE)tests/minimal/valid/basic/001_minimal.vhd
