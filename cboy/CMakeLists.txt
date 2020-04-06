set(cboy_SOURCE_FILES main.c)

if(NOT RENDERER)
    set(RENDERER OPENGL)
endif()

if(${RENDERER} STREQUAL OPENGL)
    set(OpenGL_GL_PREFERENCE GLVND)

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    list(APPEND cboy_SOURCE_FILES opengl.c keyboard.c)
    set(RENDERER_INCLUDES ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
    set(RENDERER_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

elseif(${RENDERER} STREQUAL FRAMEBUFFER)
    list(APPEND cboy_SOURCE_FILES framebuffer.c)
endif()

if(UNIX AND NOT APPLE)
	find_package(Threads)
    list(APPEND cboy_SOURCE_FILES joystick.c)
endif()

add_executable(cboy ${cboy_SOURCE_FILES})

include_directories(${cboy_SOURCE_DIR}/libcboy ${RENDERER_INCLUDES})
link_directories(${cboy_SOURCE_DIR}/libcboy)
target_link_libraries(cboy libcboy ${CMAKE_THREAD_LIBS_INIT} ${RENDERER_LIBS})