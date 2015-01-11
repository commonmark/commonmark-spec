# To get verbose output: cmake --build build --target "test" -- ARGS='-V'

# By default, we run the spec tests only if python3 is available.
# To require the spec tests, compile with -DSPEC_TESTS=1

if (SPEC_TESTS)
  find_package(PythonInterp 3 REQUIRED)
else(SPEC_TESTS)
  find_package(PythonInterp 3)
endif(SPEC_TESTS)

add_test(NAME api_test COMMAND api_test)

if (WIN32)
  file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/src WIN_DLL_DIR)
  set_tests_properties(api_test PROPERTIES
    ENVIRONMENT "PATH=${WIN_DLL_DIR};$ENV{PATH}"
    )
endif(WIN32)

IF (PYTHONINTERP_FOUND)

  add_test(html_normalization
    ${PYTHON_EXECUTABLE} "-m" "doctest"
    "${CMAKE_CURRENT_SOURCE_DIR}/normalize.py"
    )

  add_test(spectest_library
    ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec"
    "${CMAKE_SOURCE_DIR}/spec.txt" "--library-dir" "${CMAKE_BINARY_DIR}/src"
    )

  add_test(pathological_tests_library
    ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/pathological_tests.py"
    "--library-dir" "${CMAKE_BINARY_DIR}/src"
    )

  add_test(spectest_executable
    ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec" "${CMAKE_SOURCE_DIR}/spec.txt" "--program" "${CMAKE_BINARY_DIR}/src/cmark"
    )

ELSE(PYTHONINTERP_FOUND)

  message("\n*** A python 3 interpreter is required to run the spec tests.\n")
  add_test(skipping_spectests
    echo "Skipping spec tests, because no python 3 interpreter is available.")

ENDIF(PYTHONINTERP_FOUND)

