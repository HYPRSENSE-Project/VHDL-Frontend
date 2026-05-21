add_library(ast ast_nodes.cpp ast_node_factory.cpp)
target_include_directories(ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ast PUBLIC vhdl_fe::vhdl_grammar)
add_library(vhdl_fe::ast ALIAS ast)
