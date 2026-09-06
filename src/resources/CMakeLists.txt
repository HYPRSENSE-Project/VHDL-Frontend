set(VHDL_RESOURCE_ROOT "${PROJECT_SOURCE_DIR}/contrib")
file(GLOB_RECURSE VHDL_RESOURCE_FILES CONFIGURE_DEPENDS
    "${VHDL_RESOURCE_ROOT}/*.vhdl")

set(VHDL_RESOURCE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/embedded_vhdl.cpp")
add_custom_command(
    OUTPUT "${VHDL_RESOURCE_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/embed_vhdl.py"
        --root "${VHDL_RESOURCE_ROOT}" --output "${VHDL_RESOURCE_SOURCE}"
        ${VHDL_RESOURCE_FILES}
    DEPENDS "${PROJECT_SOURCE_DIR}/scripts/embed_vhdl.py" ${VHDL_RESOURCE_FILES}
    COMMENT "Embedding contrib VHDL packages"
    VERBATIM
)

add_library(embedded_vhdl STATIC "${VHDL_RESOURCE_SOURCE}")
target_include_directories(embedded_vhdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(vhdl_fe::embedded_vhdl ALIAS embedded_vhdl)
