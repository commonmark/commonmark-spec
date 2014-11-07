cmake_minimum_required(VERSION 2.8)
set(LIBRARY "commonmark")
set(HEADERS
  cmark.h
  buffer.h
  chunk.h
  references.h
  debug.h
  utf8.h
  scanners.h
  inlines.h
  html/html_unescape.h
  html/houdini.h
  )
set(LIBRARY_SOURCES
  blocks.c
  inlines.c
  print.c
  scanners.c
  scanners.re
  utf8.c
  buffer.c
  references.c
  html/html.c
  html/html_unescape.gperf
  html/houdini_href_e.c
  html/houdini_html_e.c
  html/houdini_html_u.c
  ${HEADERS}
  )

set(PROGRAM "cmark")
set(PROGRAM_SOURCES
  ${LIBRARY_SOURCES}
  main.c
  )

include_directories(. html)

if (MSVC)
    set(RE2C ${CMAKE_CURRENT_SOURCE_DIR}/re2c)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} DOS_CURRENT_SOURCE_DIR)
    add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/scanners.c
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scanners.re
                        COMMAND ${RE2C} --case-insensitive -b -i ${DOS_CURRENT_SOURCE_DIR}\\scanners.re >${DOS_CURRENT_SOURCE_DIR}\\scanners.c)
else(MSVC)
    set(RE2C re2c)
    add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/scanners.c
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scanners.re
                        COMMAND ${RE2C} --case-insensitive -b -i ${CMAKE_CURRENT_SOURCE_DIR}/scanners.re >${CMAKE_CURRENT_SOURCE_DIR}/scanners.c)
endif(MSVC)

add_executable(${PROGRAM} ${PROGRAM_SOURCES})

add_library(${LIBRARY} SHARED ${LIBRARY_SOURCES})

set_property(TARGET ${LIBRARY}
  APPEND PROPERTY MACOSX_RPATH true)

if (MSVC)
set_property(TARGET ${PROGRAM}
  APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO)

endif(MSVC)

install(TARGETS ${PROGRAM} ${LIBRARY}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  )
install(DIRECTORY ./
  DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h")

if(UNIX)
  INSTALL(FILES ../man/man1/cmark.1 DESTINATION share/man/man1)
endif(UNIX)

include (CheckIncludeFile)
CHECK_INCLUDE_FILE (stdbool.h HAVE_STDBOOL_H)

# Always compile with warnings
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")
endif()

