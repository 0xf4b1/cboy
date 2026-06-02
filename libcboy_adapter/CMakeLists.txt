# SPDX-License-Identifier: GPL-3.0-only
#
# Interface-only target: just provides the include path to the adapter header.
# The actual compiled code lives in libcboy (already built as a separate target).

add_library(libcboy_adapter INTERFACE)

target_include_directories(libcboy_adapter INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}   # for gameboy_adapter.hpp
    ${CMAKE_SOURCE_DIR}/libcboy   # for gameboy.h, controls.h, display.h, cpu.h
)

# The adapter header includes libcboy's C API, so consumers must also link
# the compiled libcboy library.
target_link_libraries(libcboy_adapter INTERFACE libcboy_c)
