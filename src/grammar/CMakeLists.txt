set(ANTLR_VERSION 4.13.2)
set(ANTLR_JAR "${CMAKE_BINARY_DIR}/tools/antlr-${ANTLR_VERSION}-complete.jar")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

if(NOT EXISTS "${ANTLR_JAR}")
    file(DOWNLOAD
        "https://www.antlr.org/download/antlr-${ANTLR_VERSION}-complete.jar"
        "${ANTLR_JAR}"
        SHOW_PROGRESS
        STATUS ANTLR_DOWNLOAD_STATUS
        TLS_VERIFY ON
    )

    list(GET ANTLR_DOWNLOAD_STATUS 0 ANTLR_DOWNLOAD_CODE)
    if(NOT ANTLR_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download ANTLR jar: ${ANTLR_DOWNLOAD_STATUS}")
    endif()
endif()

set(ANTLR4_TAG "${ANTLR_VERSION}")
set(ANTLR4_WITH_STATIC_CRT OFF)
include(ExternalAntlr4Cpp)

set(ANTLR_EXECUTABLE "${ANTLR_JAR}")
find_package(ANTLR REQUIRED)

set(ANTLR_GENERATED_ROOT "${CMAKE_CURRENT_BINARY_DIR}/antlr4cpp_generated_src")
set(ANTLR_VHDL_OUTPUT_DIR
    "${ANTLR_GENERATED_ROOT}/hdlConvertor/vhdlConvertor/vhdlParser")

antlr_target(vhdlLexer vhdlLexer.g4
    LEXER
    PACKAGE vhdl_antlr
    OUTPUT_DIRECTORY "${ANTLR_VHDL_OUTPUT_DIR}"
)

antlr_target(vhdlParser vhdlParser.g4
    PARSER
    PACKAGE vhdl_antlr
    OUTPUT_DIRECTORY "${ANTLR_VHDL_OUTPUT_DIR}"
    DEPENDS_ANTLR vhdlLexer
    COMPILE_FLAGS -lib "${ANTLR_VHDL_OUTPUT_DIR}"
)

add_library(vhdl_grammar STATIC
    ${ANTLR_vhdlLexer_CXX_OUTPUTS}
    ${ANTLR_vhdlParser_CXX_OUTPUTS}
)
add_library(vhdl_grammar::vhdl_grammar ALIAS vhdl_grammar)

target_compile_definitions(vhdl_grammar PUBLIC ANTLR4CPP_STATIC)
target_include_directories(vhdl_grammar PUBLIC "${ANTLR_GENERATED_ROOT}")
target_link_libraries(vhdl_grammar PUBLIC antlr4_static)
