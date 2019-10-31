cmake_minimum_required(VERSION 2.4)

project(cboy)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(cboy main.c cpu.c mmu.c display.c controls.c instructions/opcodes.c)

include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_link_libraries(cboy ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})