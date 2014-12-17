if (NOT MSVC)
  add_custom_target(man ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cmark.3
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cmark.1
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmark.3
    DEPENDS ${CMAKE_SOURCE_DIR}/src/cmark.h
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/make_man_page.py
    ${CMAKE_SOURCE_DIR}/src/cmark.h >
    ${CMAKE_CURRENT_BINARY_DIR}/cmark.3
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmark.1
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man1/cmark.1
    COMMAND cp
    ${CMAKE_CURRENT_SOURCE_DIR}/man1/cmark.1
    ${CMAKE_CURRENT_BINARY_DIR}/cmark.1
    )
endif(NOT MSVC)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmark.1
  DESTINATION share/man/man1)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmark.3
  DESTINATION share/man/man3)