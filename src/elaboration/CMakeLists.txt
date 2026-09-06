add_library(elaboration
    elaborator.cpp
    validator.cpp
)
target_compile_definitions(elaboration PRIVATE $<$<CONFIG:DEBUG>:_GLIBCXX_DEBUG>)
target_include_directories(elaboration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(elaboration PUBLIC vhdl_fe::parser)
add_library(vhdl_fe::elaboration ALIAS elaboration)
