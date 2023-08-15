cmake_minimum_required(VERSION 3.20.0)

if(NOT BOARD)
    message(WARNING "Use default board `k230_evb`, please run `cmake .. -DBOARD=xxx` to specify")
    set(BOARD "k230_evb")
endif()

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

project(K230_MicroPython)

set(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(K230_SDK "${CMAKE_CURRENT_SOURCE_DIR}/k230_sdk")
set(K230_SDK_OVERLAY "${CMAKE_CURRENT_SOURCE_DIR}/k230_sdk_overlay")
set(MICROPYTHON "${CMAKE_CURRENT_SOURCE_DIR}/micropython")
set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

# download submodule
# fast download default on
option(FAST_DL "Accelerate downloads in mainland China, default ON" ON)

if(${FAST_DL})
    execute_process(
        COMMAND curl --output /dev/null --silent --head --fail "https://ai.b-bug.org/k230/"
        TIMEOUT 1
        RESULT_VARIABLE IS_EXTERNAL_NETWORK
    )
    if(${IS_EXTERNAL_NETWORK})
        set(SERVER_URL "https://kendryte-download.canaan-creative.com/k230")
        set(K230_SDK_DOWNLOAD_URL "${SERVER_URL}/release/sdk/k230_sdk.tar.gz")
    else()
        set(SERVER_URL "https://ai.b-bug.org/k230")
        set(K230_SDK_DOWNLOAD_URL "${SERVER_URL}/release/sdk/github/k230_sdk.tar.gz")
    endif()

    set(MICROPYTHON_DOWNLOAD_URL "${SERVER_URL}/downloads/canmv/micropython.tar.gz")

    execute_process(
        COMMAND make -f ${SCRIPTS_DIR}/helper.mk fast_dl
            PROJECT_ROOT_DIR=${PROJECT_ROOT_DIR}
            K230_SDK_DOWNLOAD_URL=${K230_SDK_DOWNLOAD_URL}
            MICROPYTHON_DOWNLOAD_URL=${MICROPYTHON_DOWNLOAD_URL}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# sync submodule
execute_process(
    COMMAND make -f ${SCRIPTS_DIR}/helper.mk sync_submodule
        PROJECT_ROOT_DIR=${PROJECT_ROOT_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

execute_process(
    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/images
)

include("cmake/toolchain.cmake")
include("cmake/top.cmake")

add_subdirectory("k230_sdk_overlay")
add_subdirectory("micropython_port")
