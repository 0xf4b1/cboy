add_library(libcboy_c cpu.c mmu.c mbc.c display.c controls.c timer.c instructions/instructions.c instructions/cb.c gameboy.c)
target_include_directories(libcboy_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MSVC: suppress C runtime deprecation warnings for standard C functions
if(MSVC)
    target_compile_definitions(libcboy_c PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()