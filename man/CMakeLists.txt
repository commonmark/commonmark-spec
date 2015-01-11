if (NOT MSVC)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/man1/cmark.1
    DESTINATION share/man/man1)

  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/man3/cmark.3
    DESTINATION share/man/man3)
endif(NOT MSVC)

