add_executable(cboy main.c)

include_directories(${cboy_SOURCE_DIR}/libcboy)
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(cboy libcboy)

file(GLOB_RECURSE files "${cboy_SOURCE_DIR}/tests/roms/*.gb")

set(i 1)
foreach(file ${files})
    get_filename_component(filename ${file} NAME)
    message(STATUS ${filename})
    add_test(NAME "test_${i}" COMMAND cboy ${file})
    math(EXPR i "${i} + 1")
endforeach()
