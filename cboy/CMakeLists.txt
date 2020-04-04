set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Threads)

set(cboy_SOURCE_FILES main.c opengl.c keyboard.c)

if(UNIX AND NOT APPLE)
    list(APPEND cboy_SOURCE_FILES "joystick.c")
endif()

add_executable(cboy ${cboy_SOURCE_FILES})

include_directories(${cboy_SOURCE_DIR}/libcboy ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(cboy libcboy ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})