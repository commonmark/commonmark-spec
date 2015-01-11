cmake_minimum_required(VERSION 2.8)
set(LIBRARY "libcmark")
set(HEADERS
  cmark.h
  parser.h
  buffer.h
  node.h
  iterator.h
  chunk.h
  references.h
  debug.h
  bench.h
  utf8.h
  scanners.h
  inlines.h
  html_unescape.h
  houdini.h
  cmark_ctype.h
  )
set(LIBRARY_SOURCES
  cmark.c
  node.c
  iterator.c
  blocks.c
  inlines.c
  scanners.c
  scanners.re
  utf8.c
  buffer.c
  references.c
  man.c
  xml.c
  html.c
  html_unescape.gperf
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  cmark_ctype.c
  ${HEADERS}
  )

set(PROGRAM "cmark")
set(PROGRAM_SOURCES
  ${LIBRARY_SOURCES}
  main.c
  )

include_directories(. ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libcmark.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc
  DESTINATION lib/pkgconfig)

include (GenerateExportHeader)

add_executable(${PROGRAM} ${PROGRAM_SOURCES})
add_compiler_export_flags()

# Disable the PUBLIC declarations when compiling the executable:
set_target_properties(${PROGRAM} PROPERTIES
  COMPILE_FLAGS -DCMARK_STATIC_DEFINE)

# Check integrity of node structure when compiled as debug:
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg -DCMARK_DEBUG_NODES")
set(CMAKE_LINKER_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -pg")

if (${CMAKE_MAJOR_VERSION} GREATER 1 AND ${CMAKE_MINOR_VERSION} GREATER 8)
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
elseif(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif ()

add_library(${LIBRARY} SHARED ${LIBRARY_SOURCES})
set_target_properties(${LIBRARY}
  PROPERTIES OUTPUT_NAME "cmark")

set_property(TARGET ${LIBRARY}
  APPEND PROPERTY MACOSX_RPATH true)

generate_export_header(${LIBRARY}
    BASE_NAME ${PROJECT_NAME})

if (MSVC)
  set_property(TARGET ${PROGRAM}
    APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO)
endif(MSVC)

install(TARGETS ${PROGRAM} ${LIBRARY}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  )

install(FILES cmark.h ${CMAKE_CURRENT_BINARY_DIR}/cmark_export.h
  DESTINATION include
  )

# Feature tests
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckSymbolExists)
CHECK_INCLUDE_FILE(stdbool.h HAVE_STDBOOL_H)
CHECK_C_SOURCE_COMPILES(
  "int main() { __builtin_expect(0,0); return 0; }"
  HAVE___BUILTIN_EXPECT)
CHECK_C_SOURCE_COMPILES("
  int f(void) __attribute__ (());
  int main() { return 0; }
" HAVE___ATTRIBUTE__)
CHECK_SYMBOL_EXISTS(va_copy stdarg.h HAVE_VA_COPY)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Always compile with warnings
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4127 /wd4244 /wd4267 /wd4706 /wd4800 /D_CRT_SECURE_NO_WARNINGS")
elseif(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99 -pedantic -D_GNU_SOURCE")
endif()

# Compile as C++ under MSVC
if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /TP")
endif()

if($ENV{TIMER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTIMER=1")
endif($ENV{TIMER})
