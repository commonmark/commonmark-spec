CFLAGS?=-g -O3 -Wall -Wextra -std=c99 -Isrc -Wno-missing-field-initializers -fPIC $(OPTCFLAGS)
LDFLAGS?=-g -O3 -Wall -Werror -fPIC $(OPTLDFLAGS)
SRCDIR?=src
DATADIR?=data
BENCHINP?=README.md
JSMODULES=$(wildcard js/lib/*.js)
PREFIX?=/usr/local
SPEC=spec.txt
SITE=_site
SPECVERSION=$(shell grep version: $(SPEC) | sed -e 's/version: *//')

BUILDDIR=build
PROG?=$(BUILDDIR)/src/cmark

.PHONY: all spec leakcheck clean fuzztest dingus upload jshint test testjs benchjs update-site upload-site check

all: check
	mkdir -p $(BUILDDIR); cd build; cmake ..; make

install: check
	mkdir -p $(BUILDDIR); cd build; cmake ..; make install

clean:
	rm -rf $(BUILDDIR)

check:
	@cmake --version >/dev/null || (echo "You need cmake to build this program: http://www.cmake.org/download/" && exit 1)

$(PROG): all

README.html: README.md template.html
	pandoc --template template.html -S -s -t html5 -o $@ $<

spec: test spec.html

spec.md: $(SPEC)
	perl spec2md.pl < $< > $@

spec.html: spec.md template.html
	pandoc --no-highlight --number-sections --template template.html -s --toc -S $< > $@ # | perl -pe 's/␣/<span class="space"> <\/span>/g' > $@

spec.pdf: spec.md template.tex specfilter.hs
	pandoc -s $< --template template.tex \
	   --filter ./specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides

test: $(SPEC)
	perl runtests.pl $< $(PROG)

js/commonmark.js: js/lib/index.js ${JSMODULES}
	browserify --standalone commonmark $< -o $@

testjs: $(SPEC)
	node js/test.js

jshint:
	jshint ${JSMODULES}

benchjs:
	node js/bench.js ${BENCHINP}

#HTML_OBJ=$(SRCDIR)/html/html.o $(SRCDIR)/html/houdini_href_e.o $(SRCDIR)/html/houdini_html_e.o $(SRCDIR)/html/houdini_html_u.o
#
#CMARK_OBJ=$(SRCDIR)/inlines.o $(SRCDIR)/buffer.o $(SRCDIR)/blocks.o $(SRCDIR)/scanners.c $(SRCDIR)/print.o $(SRCDIR)/utf8.o $(SRCDIR)/references.o
#
#CMARK_HDR = $(SRCDIR)/cmark.h $(SRCDIR)/buffer.h $(SRCDIR)/references.h \
#           $(SRCDIR)/chunk.h $(SRCDIR)/debug.h $(SRCDIR)/utf8.h \
#           $(SRCDIR)/scanners.h $(SRCDIR)/inlines.h
#
#HTML_HDR = $(SRCDIR)/html/html_unescape.h $(SRCDIR)/html/houdini.h
#
#$(PROG): $(SRCDIR)/html/html_unescape.h $(SRCDIR)/case_fold_switch.inc $(HTML_OBJ) $(CMARK_OBJ) $(SRCDIR)/main.c
#	$(CC) $(LDFLAGS) -o $@ $(HTML_OBJ) $(CMARK_OBJ) $(SRCDIR)/main.c
#
#$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
#	re2c --case-insensitive -bis $< > $@ || (rm $@ && false)


$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl mkcasefold.pl < $< > $@

#$(SRCDIR)/html/html_unescape.h: $(SRCDIR)/html/html_unescape.gperf
#	gperf -I -t -N find_entity -H hash_entity -K entity -C -l
#	--null-strings -m5 $< > $@
#
#libcommonmark.so: $(HTML_OBJ) $(CMARK_OBJ)
#	$(CC) $(LDFLAGS) -shared -o $@ $^
#
#install: libcommonmark.so $(cmark_HDR) $(HTML_HDR)
#	install -d $(PREFIX)/lib $(PREFIX)/include/cmark/html
#	install libcommonmark.so $(PREFIX)/lib/
#	install $(cmark_HDR) $(PREFIX)/include/cmark/
#	install $(HTML_HDR) $(PREFIX)/include/cmark/html/

#CMARK_HDR = $(SRCDIR)/cmark.h $(SRCDIR)/buffer.h $(SRCDIR)/references.h \
#           $(SRCDIR)/chunk.h $(SRCDIR)/debug.h $(SRCDIR)/utf8.h \
#           $(SRCDIR)/scanners.h $(SRCDIR)/inlines.h
#
#HTML_HDR = $(SRCDIR)/html/html_unescape.h $(SRCDIR)/html/houdini.h

dingus: js/commonmark.js
	echo "Starting dingus server at http://localhost:9000" && python -m SimpleHTTPServer 9000

leakcheck: $(PROG)
	cat leakcheck.md | valgrind --leak-check=full --dsymutil=yes $(PROG)

operf: $(PROG)
	operf $(PROG) <$(BENCHINP) >/dev/null

fuzztest:
	for i in `seq 1 10`; do \
	  time cat /dev/urandom | head -c 500000 | iconv -f latin1 -t utf-8 | tee fuzz-$$i.txt | $(PROG) > /dev/null && rm fuzz-$$i.txt ; done

$(SITE)/index.html: spec.txt
	./make_site_index.sh $(SPECVERSION) | \
	  pandoc --template template.html -S -s -t html5 -o $@

$(SITE)/$(SPECVERSION)/index.html: spec.html
	mkdir -p $(SITE)/$(SPECVERSION)
	cp $< $@
	cd $(SITE); git add $(SPECVERSION)/index.html; git commit -a -m "Added version $(SPECVERSION) of spec"; cd ..

$(SITE)/%: %
	cp $< $@

update-site: $(SITE)/dingus.html $(SITE)/js/commonmark.js $(SITE)/index.html $(SITE)/$(SPECVERSION)/index.html $(SITE)/js/LICENSE

upload-site:
	cd $(SITE) ; git pull; git commit -a -m "Updated site for latest spec, js" ; git push; cd ..

distclean: clean
	-rm -f test $(SRCDIR)/*.o $(SRCDIR)/scanners.c $(SRCDIR)/html/*.o libcommonmark.so
	-rm -f js/commonmark.js
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -f spec.md fuzz.txt spec.html
