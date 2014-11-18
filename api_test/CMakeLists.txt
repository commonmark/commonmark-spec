add_executable(api_test
  harness.c
  harness.h
  main.c
)
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
)
target_link_libraries(api_test libcmark)

# Compiler flags
if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set_target_properties(api_test PROPERTIES COMPILE_FLAGS
    "-std=c99 -Wall -Wextra"
  )
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(api_test PROPERTIES COMPILE_FLAGS "/TP /W4")
endif()
