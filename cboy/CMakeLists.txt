# SPDX-License-Identifier: GPL-3.0-only

# ---- Include path and link target depend on the selected backend ----------
if(BACKEND STREQUAL "LIBCBOY")
    # Renderers include "gameboy.hpp" — the adapter provides that header.
    set(CBOY_BACKEND_INCLUDE ${CMAKE_SOURCE_DIR}/libcboy_adapter)
    set(CBOY_BACKEND_LIB     libcboy_adapter)
else()
    set(CBOY_BACKEND_INCLUDE ${CMAKE_SOURCE_DIR}/libcppboy)
    set(CBOY_BACKEND_LIB     libcboy)   # libcppboy's CMake target is "libcboy"
endif()

# ---- Renderer selection --------------------------------------------------
# Set via: cmake -DRENDERER=DIRECTX|DIRECT|OPENGL|VULKAN
# Defaults: DIRECTX on Windows, OPENGL elsewhere.

if(NOT RENDERER)
    if(WIN32)
        set(RENDERER DIRECTX)
    else()
        set(RENDERER OPENGL)
    endif()
endif()

message(STATUS "cboy renderer: ${RENDERER}")

# ---- Source files --------------------------------------------------------

set(CBOY_SOURCES main.cpp)

if(RENDERER STREQUAL "DIRECTX")
    if(NOT WIN32)
        message(FATAL_ERROR "RENDERER=DIRECTX is only supported on Windows")
    endif()
    list(APPEND CBOY_SOURCES renderer_directx.cpp)
    set(RENDERER_LIBS d3d11 dxgi d3dcompiler)

elseif(RENDERER STREQUAL "GDI")
    if(NOT WIN32)
        message(FATAL_ERROR "RENDERER=GDI is only supported on Windows")
    endif()
    list(APPEND CBOY_SOURCES renderer_gdi.cpp)
    # winmm for timeBeginPeriod/timeEndPeriod (accurate Sleep pacing)
    set(RENDERER_LIBS winmm)

elseif(RENDERER STREQUAL "OPENGL")
    list(APPEND CBOY_SOURCES renderer_opengl.cpp)

    if(WIN32)
        # ---- Windows: OpenGL ships with the OS; GLFW from the Vulkan SDK bundle ----
        if(NOT DEFINED GLFW_SDK_DIR)
            if(DEFINED ENV{VULKAN_SDK})
                get_filename_component(_vk_parent "$ENV{VULKAN_SDK}" DIRECTORY)
                set(GLFW_SDK_DIR "${_vk_parent}/glfw-3.4.bin.WIN64")
            else()
                set(GLFW_SDK_DIR "C:/VulkanSDK/glfw-3.4.bin.WIN64")
            endif()
        endif()

        message(STATUS "GLFW SDK: ${GLFW_SDK_DIR}")

        if(NOT EXISTS "${GLFW_SDK_DIR}/include/GLFW/glfw3.h")
            message(FATAL_ERROR "glfw3.h not found under ${GLFW_SDK_DIR}/include — set -DGLFW_SDK_DIR=<path>")
        endif()

        add_library(glfw STATIC IMPORTED)
        set_target_properties(glfw PROPERTIES
            IMPORTED_LOCATION             "${GLFW_SDK_DIR}/lib-vc2022/glfw3.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${GLFW_SDK_DIR}/include"
        )

        # opengl32 is part of the Windows SDK — no find_package needed
        set(RENDERER_LIBS glfw opengl32)

    else()
        # ---- Non-Windows: use system packages ----
        find_package(OpenGL REQUIRED)
        find_package(glfw3  REQUIRED)
        set(RENDERER_LIBS OpenGL::GL glfw)
    endif()

elseif(RENDERER STREQUAL "VULKAN")
    list(APPEND CBOY_SOURCES renderer_vulkan.cpp)

    if(WIN32)
        # ---- Windows: locate the Vulkan SDK and bundled GLFW manually ----
        # The SDK installer sets VULKAN_SDK; fall back to the known path.
        if(NOT DEFINED VULKAN_SDK)
            if(DEFINED ENV{VULKAN_SDK})
                set(VULKAN_SDK "$ENV{VULKAN_SDK}")
            else()
                # Default install location — override with -DVULKAN_SDK=...
                set(VULKAN_SDK "C:/VulkanSDK/1.4.341.1")
            endif()
        endif()

        # GLFW ships next to the SDK installer; override with -DGLFW_SDK_DIR=...
        if(NOT DEFINED GLFW_SDK_DIR)
            get_filename_component(_vk_parent "${VULKAN_SDK}" DIRECTORY)
            set(GLFW_SDK_DIR "${_vk_parent}/glfw-3.4.bin.WIN64")
        endif()

        message(STATUS "Vulkan SDK : ${VULKAN_SDK}")
        message(STATUS "GLFW SDK   : ${GLFW_SDK_DIR}")

        # Validate
        if(NOT EXISTS "${VULKAN_SDK}/Include/vulkan/vulkan.h")
            message(FATAL_ERROR "vulkan.h not found under ${VULKAN_SDK}/Include — set -DVULKAN_SDK=<path>")
        endif()
        if(NOT EXISTS "${GLFW_SDK_DIR}/include/GLFW/glfw3.h")
            message(FATAL_ERROR "glfw3.h not found under ${GLFW_SDK_DIR}/include — set -DGLFW_SDK_DIR=<path>")
        endif()

        # Create imported targets so the rest of the CMakeLists stays uniform
        add_library(Vulkan::Vulkan STATIC IMPORTED)
        set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_LOCATION             "${VULKAN_SDK}/Lib/vulkan-1.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_SDK}/Include"
        )

        add_library(glfw STATIC IMPORTED)
        set_target_properties(glfw PROPERTIES
            IMPORTED_LOCATION             "${GLFW_SDK_DIR}/lib-vc2022/glfw3.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${GLFW_SDK_DIR}/include"
        )

    else()
        # ---- Non-Windows: use system packages ----
        find_package(Vulkan REQUIRED)
        find_package(glfw3  REQUIRED)
    endif()

    set(RENDERER_LIBS Vulkan::Vulkan glfw)

elseif(RENDERER STREQUAL "SDL2")
    list(APPEND CBOY_SOURCES renderer_sdl2.cpp)

    if(WIN32)
        # SDL2 on Windows: expect SDL2_DIR or SDL2_SDK_DIR pointing to the
        # extracted SDL2 development package (e.g. SDL2-devel-2.x.x-VC.zip).
        # Override with -DSDL2_SDK_DIR=<path>.
        if(NOT DEFINED SDL2_SDK_DIR)
            if(DEFINED ENV{SDL2_DIR})
                set(SDL2_SDK_DIR "$ENV{SDL2_DIR}")
            else()
                # Common manual install location — adjust as needed
                set(SDL2_SDK_DIR "C:/SDL2")
            endif()
        endif()

        message(STATUS "SDL2 SDK: ${SDL2_SDK_DIR}")

        if(NOT EXISTS "${SDL2_SDK_DIR}/include/SDL2/SDL.h")
            message(FATAL_ERROR
                "SDL.h not found under ${SDL2_SDK_DIR}/include/SDL2\n"
                "Download SDL2-devel-*-VC.zip from https://libsdl.org/download-2.0.php\n"
                "and set -DSDL2_SDK_DIR=<extracted path>")
        endif()

        add_library(SDL2::SDL2 SHARED IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION             "${SDL2_SDK_DIR}/lib/x64/SDL2.dll"
            IMPORTED_IMPLIB               "${SDL2_SDK_DIR}/lib/x64/SDL2.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_SDK_DIR}/include"
        )
        add_library(SDL2::SDL2main STATIC IMPORTED)
        set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LOCATION "${SDL2_SDK_DIR}/lib/x64/SDL2main.lib"
        )
        set(RENDERER_LIBS SDL2::SDL2 SDL2::SDL2main)

    else()
        find_package(SDL2 REQUIRED)
        # SDL2 cmake config names differ across distros/versions
        if(TARGET SDL2::SDL2)
            set(RENDERER_LIBS SDL2::SDL2)
        else()
            set(RENDERER_LIBS ${SDL2_LIBRARIES})
        endif()
    endif()

elseif(RENDERER STREQUAL "METAL")
    if(NOT APPLE)
        message(FATAL_ERROR "RENDERER=METAL is only supported on Apple platforms")
    endif()
    list(APPEND CBOY_SOURCES renderer_metal.mm)
    # Metal, MetalKit, Cocoa — all system frameworks on macOS
    set(RENDERER_LIBS
        "-framework Metal"
        "-framework MetalKit"
        "-framework Cocoa"
        "-framework QuartzCore"
    )

else()
    message(FATAL_ERROR
        "Unknown RENDERER '${RENDERER}'. "
        "Choose: DIRECTX GDI OPENGL VULKAN SDL2 METAL")
endif()

# Joystick support on Linux
#if(UNIX AND NOT APPLE)
#    find_package(Threads REQUIRED)
#    list(APPEND CBOY_SOURCES joystick.c)
#    list(APPEND RENDERER_LIBS Threads::Threads)
#endif()

# ---- Target --------------------------------------------------------------

add_executable(cboy ${CBOY_SOURCES})

target_include_directories(cboy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CBOY_BACKEND_INCLUDE}
)

# SDL2 on non-Windows may need include dirs from find_package variables
if(RENDERER STREQUAL "SDL2" AND NOT WIN32 AND DEFINED SDL2_INCLUDE_DIRS)
    target_include_directories(cboy PRIVATE ${SDL2_INCLUDE_DIRS})
endif()

target_link_libraries(cboy PRIVATE
    ${CBOY_BACKEND_LIB}
    ${RENDERER_LIBS}
)

# Windows: use WinMain subsystem
if(WIN32 AND MSVC)
    set_target_properties(cboy PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
elseif(WIN32 AND NOT MSVC)
    target_link_options(cboy PRIVATE -mwindows)
endif()

# SDL2 on Windows: copy SDL2.dll next to the exe after build
if(RENDERER STREQUAL "SDL2" AND WIN32 AND DEFINED SDL2_SDK_DIR)
    add_custom_command(TARGET cboy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_SDK_DIR}/lib/x64/SDL2.dll"
            "$<TARGET_FILE_DIR:cboy>/SDL2.dll"
    )
endif()

# Compile-time renderer define (useful for #ifdef guards if ever needed)
target_compile_definitions(cboy PRIVATE CBOY_RENDERER_${RENDERER})
