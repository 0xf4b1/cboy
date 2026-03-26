set(cboy_SOURCE_FILES main.cc)

if(NOT RENDERER)
    set(RENDERER OPENGL)
endif()

if(${RENDERER} STREQUAL OPENGL)
    set(OpenGL_GL_PREFERENCE GLVND)

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    list(APPEND cboy_SOURCE_FILES opengl.cc keyboard.cc renderer_opengl.cpp)
    set(RENDERER_INCLUDES ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
    if(GLUT_LIBRARIES)
        set(RENDERER_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
    else()
        set(RENDERER_LIBS ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    endif()

elseif(${RENDERER} STREQUAL FRAMEBUFFER)
    list(APPEND cboy_SOURCE_FILES framebuffer.c)
endif()

if(UNIX AND NOT APPLE)
	find_package(Threads)
    list(APPEND cboy_SOURCE_FILES joystick.c)
endif()

add_executable(cboy ${cboy_SOURCE_FILES})

include_directories(${cboy_SOURCE_DIR}/libcppboy ${RENDERER_INCLUDES})
link_directories(${cboy_SOURCE_DIR}/libcppboy)
target_link_libraries(cboy libcboy ${CMAKE_THREAD_LIBS_INIT} ${RENDERER_LIBS})