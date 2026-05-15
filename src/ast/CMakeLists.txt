add_library(ast INTERFACE)
target_include_directories(ast INTERFACE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_library(vhdl_fe::ast ALIAS ast)
