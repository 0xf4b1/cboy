cmake_minimum_required(VERSION 4.3.0)

set (CMAKE_CXX_STANDARD 11)

project(cboy)

add_compile_options(-Wall -Wextra)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG "-O1 -framework OpenGL -framework GLUT -arch arm64")
set(CMAKE_C_FLAGS_RELEASE "-Ofast -DNDEBUG -framework OpenGL -framework GLUT -arch arm64")
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -framework OpenGL -framework GLUT -arch arm64")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG -framework OpenGL -framework GLUT -arch arm64")

set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_subdirectory(libcboy)

if(SWITCH)

  add_subdirectory(nxboy)

else()

  add_subdirectory(cboy)

  if(TEST)
    enable_testing()
    add_subdirectory(tests)
  endif()

endif()