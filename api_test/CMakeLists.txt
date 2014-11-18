add_executable(api_test
  harness.c
  main.c
)
target_include_directories(api_test
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src
)
target_link_libraries(api_test libcmark)

