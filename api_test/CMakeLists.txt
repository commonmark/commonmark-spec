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

