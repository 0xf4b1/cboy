cmake_minimum_required(VERSION 4.3.0)

set(CMAKE_CXX_STANDARD 23)

project(cboy)

add_compile_options(-Wall -Wextra -Wno-poison-system-directories -Wno-unused-private-field)

if(APPLE)
  add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_DEBUG "-O3 -ffast-math -arch arm64")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -arch arm64")
set(CMAKE_CXX_FLAGS_DEBUG "-O3 -ffast-math -arch arm64")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -arch arm64")

set(CMAKE_STATIC_LIBRARY_PREFIX "")

# add_subdirectory(libcboy)
add_subdirectory(libcppboy)

if(SWITCH)

  add_subdirectory(nxboy)

else()

  add_subdirectory(cboy)

  if(TEST)
    enable_testing()
    add_subdirectory(tests)
  endif()

endif()