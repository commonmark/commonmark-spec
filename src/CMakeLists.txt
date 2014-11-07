set(LIBRARY_cmark "commonmark")
set(LIBRARY_cmark_SOURCES   blocks.c
                            inlines.c
                            debug.h
                            print.c
                            scanners.h
                            scanners.c
                            scanners.re
                            cmark.h
                            utf8.h
                            utf8.c
                            buffer.h
                            buffer.c
                            references.h
                            references.c
                            html/html.c
                            html/html_unescape.h
                            html/html_unescape.gperf
                            html/houdini.h
                            html/houdini_href_e.c
                            html/houdini_html_e.c
                            html/houdini_html_u.c
                            )
set(PROGRAM_cmark "cmark")
set(PROGRAM_cmark_SOURCES   ${LIBRARY_cmark_SOURCES}
                            main.c)

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

add_executable(${PROGRAM_cmark}
                ${PROGRAM_cmark_SOURCES})

add_library(${LIBRARY_cmark} SHARED ${LIBRARY_cmark_SOURCES})

set_property(TARGET ${LIBRARY_cmark}
  APPEND PROPERTY MACOSX_RPATH true)

if (MSVC)
set_property(TARGET ${PROGRAM_cmark}
                     APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO)
# if sometimes libs are needed ...
#target_link_libraries(${PROGRAM_chronolog})
endif(MSVC)
install(TARGETS ${PROGRAM_cmark}
  RUNTIME DESTINATION bin)
install(TARGETS ${LIBRARY_cmark}
  LIBRARY DESTINATION lib)
