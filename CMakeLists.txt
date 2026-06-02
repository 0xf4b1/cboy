cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 23)

project(cboy)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  set(CMAKE_C_FLAGS_DEBUG "/O2")
  set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "/O2")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
else()
  set(CMAKE_C_FLAGS_DEBUG "-O1")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-O1")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
endif()

set(CMAKE_STATIC_LIBRARY_PREFIX "")

# ---- Backend selection ---------------------------------------------------
# -DBACKEND=LIBCPPBOY  (default) — use the C++ rewrite
# -DBACKEND=LIBCBOY            — use the original C library via a thin adapter

if(NOT BACKEND)
    set(BACKEND LIBCPPBOY)
endif()

message(STATUS "cboy backend: ${BACKEND}")

if(BACKEND STREQUAL "LIBCPPBOY")
    add_subdirectory(libcppboy)
    # The cboy exe target links against "libcboy" (the libcppboy CMake target name).
    # Nothing else to do — renderers include from libcppboy directly.

elseif(BACKEND STREQUAL "LIBCBOY")
    # Build the original C library under a distinct CMake target name.
    add_subdirectory(libcboy)           # defines target: libcboy_c
    add_subdirectory(libcboy_adapter)   # defines INTERFACE target: libcboy_adapter

else()
    message(FATAL_ERROR "Unknown BACKEND '${BACKEND}'. Choose: LIBCPPBOY LIBCBOY")
endif()

if(SWITCH)

  add_subdirectory(nxboy)

else()

  add_subdirectory(cboy)

  if(TEST)
    enable_testing()
    add_subdirectory(tests)
  endif()

endif()