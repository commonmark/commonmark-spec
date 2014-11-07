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
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} DOS_CURRENT_SOURCE_DIR)
    add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/scanners.c
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scanners.re
                        COMMAND ${RE2C} --case-insensitive -b -i ${DOS_CURRENT_SOURCE_DIR}\\scanners.re >${DOS_CURRENT_SOURCE_DIR}\\scanners.c)
else(MSVC)
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
  LIBRARY DESTINATION lib)
