set(MANDEST ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${MANDEST}/man1)

add_custom_target(manpages ALL
  DEPENDS ${MANDEST}/man1/cmark.1
)

# This is a temporary fallback until we get a cmark-based
# build procedure for the man pages:
add_custom_command(OUTPUT ${MANDEST}/man1/cmark.1
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/man1/cmark.1
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/man1/cmark.1
             ${MANDEST}/man1/cmark.1 VERBATIM
)

# add_custom_command(OUTPUT ${MANDEST}/man1/cmark.1
#   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmark.1.md
#   COMMAND pandoc "-s" "-t" "man"
#   ${CMAKE_CURRENT_SOURCE_DIR}/cmark.1.md
#   "-o" ${MANDEST}/man1/cmark.1
#   VERBATIM)

INSTALL(FILES ${MANDEST}/man1/cmark.1 DESTINATION share/man/man1)

# TODO:  create cmark.3 man page.
# file(MAKE_DIRECTORY ${MANDEST}/man3)
# INSTALL(FILES ${MANDEST}/man3/cmark.3 DESTINATION share/man/man3)
