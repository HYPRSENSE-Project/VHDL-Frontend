add_library(ast ast_nodes.cpp)

target_include_directories(ast PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_library(ast::ast ALIAS ast)
