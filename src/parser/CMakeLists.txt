add_library(parser
    encoding_conversions.cpp
    syntax_error_logger.cpp
    context_parser.cpp
)

target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser PUBLIC vhdl_fe::vhdl_grammar vhdl_fe::ast)
add_library(vhdl_fe::parser ALIAS parser)
