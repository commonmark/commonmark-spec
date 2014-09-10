set(PROGRAM_stmd "stmd")
set(PROGRAM_stmd_SOURCES    blocks.c
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
                            scanners.c
                            scanners.re
                            stmd.h
                            utf8.c
                            utf8.h
                            uthash.h 
)

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

add_executable(${PROGRAM_stmd} 
                ${PROGRAM_stmd_SOURCES})

if (MSVC)
set_property(TARGET PROGRAM_stmd 
                     APPEND PROPERTY LINK_FLAGS /INCREMENTAL:NO) 
# if sometimes libs are needed ...
#target_link_libraries(${PROGRAM_chronolog})
endif(MSVC)
install(TARGETS PROGRAM_stmd
RUNTIME DESTINATION bin 
BUNDLE DESTINATION Applications)


