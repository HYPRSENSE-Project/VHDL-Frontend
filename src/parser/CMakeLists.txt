add_library(parser 
    encodingConversions.cpp
    notImplementedLogger.cpp 
    syntaxErrorLogger.cpp
    context/context_parser.cpp
)

target_include_directories(parser PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(parser PUBLIC ast vhdl_grammar::vhdl_grammar)
add_library(parser::parser ALIAS parser)
