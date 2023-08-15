set(TOOLCHAIN_PREFIX "${K230_SDK}/toolchain/riscv64-linux-musleabi_for_x86_64-pc-linux-gnu/bin/riscv64-unknown-linux-musl-")

set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_C_AR "${TOOLCHAIN_PREFIX}ar")
set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}objdump")
set(CMAKE_READELF "${TOOLCHAIN_PREFIX}readelf")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp -march=rv64imafdcv -mabi=lp64d -mcmodel=medany")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
