foreach(required_var IN ITEMS HOST_PYTHON HDLCONVERTOR_SOURCE_DIR HDLCONVERTOR_BINARY_DIR HDLCONVERTOR_VENV_DIR HDLCONVERTOR_BUILD_TYPE HDLCONVERTOR_PYTHON_PACKAGE MODE)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR "Missing required variable: ${required_var}")
    endif()
endforeach()

if(HDLCONVERTOR_BUILD_TYPE STREQUAL "Debug")
    set(HDLCONVERTOR_MESON_BUILDTYPE "debug")
elseif(HDLCONVERTOR_BUILD_TYPE STREQUAL "Release")
    set(HDLCONVERTOR_MESON_BUILDTYPE "release")
elseif(HDLCONVERTOR_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(HDLCONVERTOR_MESON_BUILDTYPE "debugoptimized")
elseif(HDLCONVERTOR_BUILD_TYPE STREQUAL "MinSizeRel")
    set(HDLCONVERTOR_MESON_BUILDTYPE "minsize")
else()
    message(FATAL_ERROR
        "Unsupported HDLCONVERTOR_BUILD_TYPE value: ${HDLCONVERTOR_BUILD_TYPE}")
endif()

if(HDLCONVERTOR_PYTHON_PACKAGE)
    set(HDLCONVERTOR_PYTHON_PACKAGE_OPTION "true")
else()
    set(HDLCONVERTOR_PYTHON_PACKAGE_OPTION "false")
endif()

if(WIN32)
    set(HDLCONVERTOR_VENV_BIN "${HDLCONVERTOR_VENV_DIR}/Scripts")
    set(HDLCONVERTOR_VENV_PYTHON "${HDLCONVERTOR_VENV_BIN}/python.exe")
    set(PATH_SEP ";")
else()
    set(HDLCONVERTOR_VENV_BIN "${HDLCONVERTOR_VENV_DIR}/bin")
    set(HDLCONVERTOR_VENV_PYTHON "${HDLCONVERTOR_VENV_BIN}/python")
    set(PATH_SEP ":")
endif()

if(NOT EXISTS "${HDLCONVERTOR_VENV_PYTHON}")
    execute_process(
        COMMAND "${HOST_PYTHON}" -m venv "${HDLCONVERTOR_VENV_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

execute_process(
    COMMAND "${HDLCONVERTOR_VENV_PYTHON}" -c "import mesonbuild, Cython"
    RESULT_VARIABLE hdlconvertor_tools_ready
)

if(NOT hdlconvertor_tools_ready EQUAL 0)
    execute_process(
        COMMAND "${HDLCONVERTOR_VENV_PYTHON}" -m pip install meson cython
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(HDLCONVERTOR_ENV_PATH "${HDLCONVERTOR_VENV_BIN}${PATH_SEP}$ENV{PATH}")

if(MODE STREQUAL "configure")
    if(EXISTS "${HDLCONVERTOR_BINARY_DIR}/build.ninja")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E env "PATH=${HDLCONVERTOR_ENV_PATH}"
                    "${HDLCONVERTOR_VENV_PYTHON}" -m mesonbuild.mesonmain
                    configure "${HDLCONVERTOR_BINARY_DIR}"
                    -Dbuildtype=${HDLCONVERTOR_MESON_BUILDTYPE}
                    -Dpython_package=${HDLCONVERTOR_PYTHON_PACKAGE_OPTION}
            WORKING_DIRECTORY "${HDLCONVERTOR_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(STATUS
            "hdlConvertor Meson build configured at ${HDLCONVERTOR_BINARY_DIR} with buildtype ${HDLCONVERTOR_MESON_BUILDTYPE} and python_package=${HDLCONVERTOR_PYTHON_PACKAGE_OPTION}")
        return()
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${HDLCONVERTOR_ENV_PATH}"
                "${HDLCONVERTOR_VENV_PYTHON}" -m mesonbuild.mesonmain
                setup "${HDLCONVERTOR_BINARY_DIR}" "${HDLCONVERTOR_SOURCE_DIR}"
                --buildtype "${HDLCONVERTOR_MESON_BUILDTYPE}"
                -Dpython_package=${HDLCONVERTOR_PYTHON_PACKAGE_OPTION}
        WORKING_DIRECTORY "${HDLCONVERTOR_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
elseif(MODE STREQUAL "build")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${HDLCONVERTOR_ENV_PATH}"
                "${HDLCONVERTOR_VENV_PYTHON}" -m mesonbuild.mesonmain
                compile -C "${HDLCONVERTOR_BINARY_DIR}"
        WORKING_DIRECTORY "${HDLCONVERTOR_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
else()
    message(FATAL_ERROR "Unsupported MODE value: ${MODE}")
endif()
