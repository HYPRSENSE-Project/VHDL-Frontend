add_library(ast ast_nodes.cpp)
target_include_directories(ast INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(vhdl_fe::ast ALIAS ast)
