cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(LIBCBOY "../../../../../libcboy")
add_library(cboy STATIC ${LIBCBOY}/cpu.c ${LIBCBOY}/mmu.c ${LIBCBOY}/mbc.c ${LIBCBOY}/display.c ${LIBCBOY}/controls.c ${LIBCBOY}/instructions/opcodes.c ${LIBCBOY}/gameboy.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(anboy SHARED main.cpp display.cpp controls.cpp)

target_include_directories(anboy PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)

target_include_directories(anboy PRIVATE ${LIBCBOY})

# add lib dependencies
target_link_libraries(anboy
    android
    native_app_glue
    cboy
    EGL
    GLESv1_CM
    log)
