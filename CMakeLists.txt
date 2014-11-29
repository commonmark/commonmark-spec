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
add_subdirectory(api_test)

if(UNIX)
  INSTALL(FILES man/man1/cmark.1 DESTINATION share/man/man1)
  INSTALL(FILES man/man3/cmark.3 DESTINATION share/man/man3)
endif(UNIX)

enable_testing()

# To get verbose output: cmake --build build --target "test" -- ARGS='-V'
add_test(spectest_library
   python "${CMAKE_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec"
   "${CMAKE_SOURCE_DIR}/spec.txt" "--library-dir" "${CMAKE_BINARY_DIR}/src"
)
add_test(pathological_tests_library
   python "${CMAKE_SOURCE_DIR}/pathological_tests.py"
   "--library-dir" "${CMAKE_BINARY_DIR}/src"
)
add_test(NAME api_test COMMAND api_test)
if (WIN32)
  file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/src WIN_DLL_DIR)
  set_tests_properties(api_test PROPERTIES
    ENVIRONMENT "PATH=${WIN_DLL_DIR};$ENV{PATH}"
  )
endif(WIN32)
add_test(spectest_executable
   python "${CMAKE_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec" "${CMAKE_SOURCE_DIR}/spec.txt" "--program" "${CMAKE_BINARY_DIR}/src/cmark"
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build, options are: Debug Release." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
