add_library(parser
    encoding_conversions.cpp
    not_implemented_logger.cpp
    syntax_error_logger.cpp
    context_parser.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(parser PUBLIC ast vhdl_grammar::vhdl_grammar)
add_library(parser::parser ALIAS parser)
