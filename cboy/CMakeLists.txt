set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Threads)

add_executable(cboy main.c opengl.c keyboard.c joystick.c)

include_directories(${cboy_SOURCE_DIR}/libcboy ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(cboy libcboy ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})