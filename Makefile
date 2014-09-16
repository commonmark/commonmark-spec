CFLAGS=-g -O3 -Wall -Wextra -std=c99 -Isrc $(OPTFLAGS)
LDFLAGS=-g -O3 -Wall -Werror
SRCDIR=src
DATADIR=data

PROG=./stmd

.PHONY: all oldtests test spec benchjs testjs
all: $(SRCDIR)/case_fold_switch.c $(PROG) lib

README.html: README.md template.html
	pandoc --template template.html -S -s -t html5 -o $@ $<

spec: test spec.html

spec.md: spec.txt
	perl spec2md.pl < $< > $@

spec.html: spec.md template.html
	pandoc --no-highlight --number-sections --template template.html -s --toc -S $< > $@ # | perl -pe 's/␣/<span class="space"> <\/span>/g' > $@

narrative.html: narrative.md template.html
	pandoc --template template.html -s -S $< -o $@

spec.pdf: spec.md template.tex specfilter.hs
	pandoc -s $< --template template.tex \
	   --filter ./specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides

oldtests:
	make -C oldtests --quiet clean all

test: spec.txt
	perl runtests.pl $< $(PROG)

testjs: spec.txt
	node js/test.js
#	perl runtests.pl js/markdown $<

benchjs:
	node js/bench.js ${BENCHINP}

$(PROG): $(SRCDIR)/main.c $(SRCDIR)/inlines.o $(SRCDIR)/blocks.o $(SRCDIR)/detab.o $(SRCDIR)/bstrlib.o $(SRCDIR)/scanners.o $(SRCDIR)/print.o $(SRCDIR)/html.o $(SRCDIR)/utf8.o
	$(CC) $(LDFLAGS) -o $@ $^

lib_test: $(SRCDIR)/lib_test.c libstmd.so
	$(CC) $(LDFLAGS) -o $@ $(SRCDIR)/lib_test.c -L. -lstmd -Wl,-rpath,.

lib: libstmd.so

libstmd.so: $(SRCDIR)/inlines.o $(SRCDIR)/blocks.o $(SRCDIR)/detab.o $(SRCDIR)/bstrlib.o $(SRCDIR)/scanners.o $(SRCDIR)/print.o $(SRCDIR)/html.o $(SRCDIR)/utf8.o
	gcc -DLIB -shared $(SRCDIR)/inlines.o $(SRCDIR)/blocks.o $(SRCDIR)/detab.o $(SRCDIR)/bstrlib.o $(SRCDIR)/scanners.o $(SRCDIR)/print.o $(SRCDIR)/html.o $(SRCDIR)/utf8.o -o libstmd.so

$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	re2c --case-insensitive -bis $< > $@ || (rm $@ && false)

$(SRCDIR)/case_fold_switch.c: $(DATADIR)/CaseFolding-3.2.0.txt
	perl mkcasefold.pl < $< > $@

.PHONY: leakcheck clean fuzztest dingus upload

dingus:
	cd js && echo "Starting dingus server at http://localhost:9000" && python -m SimpleHTTPServer 9000

leakcheck: $(PROG)
	cat oldtests/*/*.markdown | valgrind --leak-check=full --dsymutil=yes $(PROG)

fuzztest:
	for i in `seq 1 10`; do \
	  time cat /dev/urandom | head -c 100000 | iconv -f latin1 -t utf-8 | $(PROG) >/dev/null; done

update-site: spec.html narrative.html
	cp spec.html _site/
	cp narrative.html _site/index.html
	cp -r js/* _site/js/
	(cd _site ; git pull ; git commit -a -m "Updated site for latest spec, narrative, js" ; git push; cd ..)

clean:
	-rm test $(SRCDIR)/*.o $(SRCDIR)/scanners.c $(PROG) lib_test libstmd.so
	-rm -r *.dSYM
	-rm README.html
	-rm spec.md fuzz.txt spec.html
