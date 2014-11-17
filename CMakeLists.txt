project(cmark)
cmake_minimum_required(VERSION 2.8)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Do not build in-source.\nPlease remove CMakeCache.txt and the CMakeFiles/ directory.\nThen: mkdir build ; cd build ; cmake .. ; make")
endif()

set(PROJECT_NAME "cmark")

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

add_subdirectory(src)

if(UNIX)
  INSTALL(FILES man/man1/cmark.1 DESTINATION share/man/man1)
endif(UNIX)

enable_testing()

# To get verbose output: cmake --build build --target "test" -- ARGS='-V'
add_test(spectest
   perl "${CMAKE_SOURCE_DIR}/runtests.pl" "${CMAKE_SOURCE_DIR}/spec.txt" "${CMAKE_BINARY_DIR}/src/cmark"
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
