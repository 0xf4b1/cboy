# SPDX-License-Identifier: GPL-3.0-only

# ---- Test executable -----------------------------------------------------
# Build a headless test runner against whichever backend is selected.
# The test ROMs communicate pass/fail via the Game Boy serial port:
#   'P' (first char of "Passed") → exit(0)
#   'F' (first char of "Failed") → exit(1)

if(BACKEND STREQUAL "LIBCBOY")
    # C test runner — uses the original libcboy C API directly.
    add_executable(cboy_test main.c)
    target_include_directories(cboy_test PRIVATE
        ${CMAKE_SOURCE_DIR}/libcboy
    )
    target_link_libraries(cboy_test PRIVATE libcboy_c)
    if(MSVC)
        target_compile_definitions(cboy_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
else()
    # C++ test runner — uses the libcppboy cboy::Gameboy API.
    add_executable(cboy_test main_cpp.cpp)
    target_include_directories(cboy_test PRIVATE
        ${CMAKE_SOURCE_DIR}/libcppboy
    )
    target_link_libraries(cboy_test PRIVATE libcboy)
endif()

# ---- Register one CTest per ROM file -------------------------------------
file(GLOB_RECURSE rom_files "${CMAKE_SOURCE_DIR}/tests/roms/*.gb")

if(NOT rom_files)
    message(WARNING "No test ROMs found in tests/roms/ — no tests registered.")
endif()

set(i 1)
foreach(rom ${rom_files})
    get_filename_component(rom_name ${rom} NAME_WE)
    get_filename_component(rom_ext  ${rom} EXT)

    # Sanitise name: replace characters that break CTest command parsing
    string(REPLACE " " "_" safe_name "${rom_name}")
    string(REPLACE "(" "" safe_name "${safe_name}")
    string(REPLACE ")" "" safe_name "${safe_name}")
    string(REPLACE "," "" safe_name "${safe_name}")

    # Copy ROM to a safe filename in the build dir so CTest can pass it cleanly
    set(safe_rom "${CMAKE_CURRENT_BINARY_DIR}/${safe_name}${rom_ext}")
    configure_file("${rom}" "${safe_rom}" COPYONLY)

    add_test(
        NAME    "test_${i}_${safe_name}"
        COMMAND cboy_test "${safe_rom}"
    )
    set_tests_properties("test_${i}_${safe_name}" PROPERTIES TIMEOUT 60)
    math(EXPR i "${i} + 1")
endforeach()
