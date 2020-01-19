find_package(Threads)

if(NOT RENDERER)
    set(RENDERER OPENGL)
endif()

if(${RENDERER} STREQUAL OPENGL)
    set(OpenGL_GL_PREFERENCE GLVND)

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    set(RENDERER_SOURCES opengl.c keyboard.c)
    set(RENDERER_INCLUDES ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
    set(RENDERER_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

elseif(${RENDERER} STREQUAL FRAMEBUFFER)
    set(RENDERER_SOURCES framebuffer.c)
endif()

add_executable(cboy main.c joystick.c ${RENDERER_SOURCES})

include_directories(${cboy_SOURCE_DIR}/libcboy ${RENDERER_INCLUDES})
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(cboy libcboy ${CMAKE_THREAD_LIBS_INIT} ${RENDERER_LIBS})