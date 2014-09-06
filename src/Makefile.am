bin_PROGRAMS = stmd
stmd_SOURCES = scanners.c \
               blocks.c \
               bstrlib.c \
               detab.c \
               html.c \
               inlines.c \
               print.c \
               utf8.c \
               main.c
stmd_LDFLAGS = 
stmd_CFLAGS = -O3 -Wall -Wextra -std=c99 -Isrc $(OPTFLAGS)

scanners.c: scanners.re
	re2c --case-insensitive -bis $< > $@ || (rm $@ && false)
