# SPDX-License-Identifier: GPL-3.0-only

# Library source files
set(libcboy_SOURCES
    cpu.cpp
    mmu.cpp
    mbc.cpp
    display.cpp
    controls.cpp
    timer.cpp
    gameboy.cpp
    instructions/instructions.cpp
    instructions/cb.cpp
)

# Library header files for IDE organization
set(libcboy_HEADERS
    cpu.hpp
    mmu.hpp
    mbc.hpp
    display.hpp
    controls.hpp
    timer.hpp
    gameboy.hpp
    instructions/instructions.hpp
    instructions/cb.hpp
)

# Create the library
add_library(libcboy STATIC ${libcboy_SOURCES} ${libcboy_HEADERS})

# Set public include directories
target_include_directories(libcboy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Enable all compiler warnings for the library
if(MSVC)
    target_compile_options(libcboy PRIVATE /W4)
else()
    target_compile_options(libcboy PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
    )
endif()

# Optional: Enable clang-specific warnings if using clang
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(libcboy PRIVATE
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
    )
endif()

# Optional: Enable GCC-specific warnings if using GCC
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(libcboy PRIVATE
        -Wuseless-cast
        -Wlogical-op
        -Wduplicated-cond
    )
endif()

# Set library properties
set_target_properties(libcboy PROPERTIES
    OUTPUT_NAME "cboy"
    VERSION 1.0.0
    SOVERSION 1
)

# Link against standard C++ library (explicit for clarity, GCC only)
if(NOT MSVC)
    target_link_libraries(libcboy PUBLIC stdc++)
endif()

# Optional: Add sanitizers for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT MSVC)
        target_compile_options(libcboy PRIVATE)
        target_link_options(libcboy PRIVATE)
    endif()
endif()

# Print library configuration
message(STATUS "libcboy configured:")
message(STATUS "  Sources: ${libcboy_SOURCES}")
message(STATUS "  Headers: ${libcboy_HEADERS}")