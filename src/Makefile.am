bin_PROGRAMS = cm
cm_SOURCES = scanners.c \
               blocks.c \
               bstrlib.c \
               detab.c \
               html.c \
               inlines.c \
               print.c \
               utf8.c \
               main.c

scanners.c: scanners.re
	re2c --case-insensitive -bis $< > $@ || (rm $@ && false)
