set(ASAN OFF)
set(UBSAN OFF)
set(PIE ON)

list(APPEND CMAKE_MODULE_PATH "${cboy_SOURCE_DIR}/nxboy/cmake")
find_package(LIBNX REQUIRED)

set(BIN_TARGET nxboy.elf)

include(nx-utils)
build_switch_binaries(${BIN_TARGET})

add_executable(${BIN_TARGET} main.c)

include_directories(${cboy_SOURCE_DIR}/libcboy)
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(${BIN_TARGET} PRIVATE switch::libnx libcboy nx)