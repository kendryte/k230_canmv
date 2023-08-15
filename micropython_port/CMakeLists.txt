cmake_minimum_required(VERSION 3.20.0)

set(MICROPY_TARGET micropython)
set(MICROPY_DIR ${PROJECT_ROOT}/micropython)
set(MICROPY_PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MICROPY_BOARD_DIR ${MICROPY_PORT_DIR}/boards/${BOARD})

include(${MICROPY_DIR}/py/py.cmake)
include(${MICROPY_DIR}/extmod/extmod.cmake)

set(MICROPY_SOURCE_PORT
    core/main.c
    core/mphalport.c
    core/gccollect.c
    core/input.c
)
list(TRANSFORM MICROPY_SOURCE_PORT PREPEND ${MICROPY_PORT_DIR}/)

set(MICROPY_SOURCE_SHARED
    runtime/gchelper_generic.c
    timeutils/timeutils.c
    readline/readline.c
    runtime/pyexec.c
    # runtime/mpirq.c
)
list(TRANSFORM MICROPY_SOURCE_SHARED PREPEND ${MICROPY_DIR}/shared/)

set(MICROPY_SOURCE_LIB
)
list(TRANSFORM MICROPY_SOURCE_LIB PREPEND ${MICROPY_DIR}/lib/)

LIST(APPEND MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_SHARED}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_PORT}
)

LIST(APPEND MPY_INC_DIRS
    ${MICROPY_INC_CORE}
    ${MICROPY_BOARD_DIR}
    ${MICROPY_PORT_DIR}
    ${AUTOCONF_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    "${MICROPY_PORT_DIR}/include"
    "${MICROPY_PORT_DIR}/include/common"
    "${MICROPY_PORT_DIR}/include/core"
    "${MICROPY_PORT_DIR}/include/wrapper"
)

add_executable(${MICROPY_TARGET} ${MICROPY_SOURCE_QSTR})

foreach(dir ${MPY_INC_DIRS})
	target_include_directories(${MICROPY_TARGET} PRIVATE ${dir})
endforeach()

LIST(APPEND MPY_DEFS
    NDEBUG
)
foreach(def ${MPY_DEFS})
    target_compile_definitions(${MICROPY_TARGET} PRIVATE ${def})
endforeach()

include(${MICROPY_DIR}/py/mkrules.cmake)

set_property(
    TARGET ${MICROPY_TARGET}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${MICROPY_GENHDR_DIR}
)

target_link_libraries(${MICROPY_TARGET}
    "-T ${CMAKE_CURRENT_SOURCE_DIR}/core/link.lds --static"
)

add_custom_command(
    TARGET ${MICROPY_TARGET}
    POST_BUILD
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/images/app
    COMMAND cp $<TARGET_FILE:${MICROPY_TARGET}> ${PROJECT_BINARY_DIR}/images/app
)

add_dependencies(${MICROPY_TARGET} _rt-smart-kernel)
