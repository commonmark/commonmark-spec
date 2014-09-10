set(PROGRAM_stmd "stmd")
set(PROGRAM_stmd_SOURCES blocks.c
inlines.c
main.c
debug.h
detab.c
bstrlib.c
bstrlib.h
getopt.c
html.c


print.c
scanners.h
scanners.re
stmd.h
utf8.c
utf8.h
uthash.h 
)

add_executable(${PROGRAM_stmd} 
                ${PROGRAM_stmd_SOURCES})

if (MSVC)
set_property(TARGET ${PROGRAM_chronolog} 
					 APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO) 
target_link_libraries(${PROGRAM_chronolog} gdiplus.lib)
endif(MSVC)
install(TARGETS ${PROGRAM_chronolog}
RUNTIME DESTINATION bin 
BUNDLE DESTINATION Applications)


