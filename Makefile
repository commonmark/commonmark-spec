CFLAGS?=-g -O3 -Wall -Wextra -std=c99 -Isrc $(OPTFLAGS)
LDFLAGS?=-g -O3 -Wall -Werror
SRCDIR?=src
DATADIR?=data

PROG?=./stmd

.PHONY: all oldtests test spec benchjs testjs
all: $(SRCDIR)/case_fold_switch.inc $(PROG)

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

HTML_OBJ=$(SRCDIR)/html/html.o $(SRCDIR)/html/houdini_href_e.o $(SRCDIR)/html/houdini_html_e.o $(SRCDIR)/html/houdini_html_u.o
STMD_OBJ=$(SRCDIR)/inlines.o $(SRCDIR)/buffer.o $(SRCDIR)/blocks.o $(SRCDIR)/scanners.c $(SRCDIR)/print.o $(SRCDIR)/utf8.o $(SRCDIR)/references.c

$(PROG): $(SRCDIR)/html/html_unescape.h $(SRCDIR)/case_fold_switch.inc $(HTML_OBJ) $(STMD_OBJ) $(SRCDIR)/main.c
	$(CC) $(LDFLAGS) -o $@ $(HTML_OBJ) $(STMD_OBJ) $(SRCDIR)/main.c

$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	re2c --case-insensitive -bis $< > $@ || (rm $@ && false)

$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl mkcasefold.pl < $< > $@

$(SRCDIR)/html/html_unescape.h: $(SRCDIR)/html/html_unescape.gperf
	gperf -I -t -N find_entity -H hash_entity -K entity -C -l --null-strings -m5 $< > $@

.PHONY: leakcheck clean fuzztest dingus upload

dingus:
	cd js && echo "Starting dingus server at http://localhost:9000" && python -m SimpleHTTPServer 9000

leakcheck: $(PROG)
	cat oldtests/*/*.markdown | valgrind --leak-check=full --dsymutil=yes $(PROG)

operf: $(PROG)
	operf $(PROG) <bench.md >/dev/null

fuzztest:
	for i in `seq 1 10`; do \
	  time cat /dev/urandom | head -c 100000 | iconv -f latin1 -t utf-8 | $(PROG) >/dev/null; done

update-site: spec.html narrative.html
	cp spec.html _site/
	cp narrative.html _site/index.html
	cp -r js/* _site/js/
	(cd _site ; git pull ; git commit -a -m "Updated site for latest spec, narrative, js" ; git push; cd ..)

clean:
	-rm -f test $(SRCDIR)/*.o $(SRCDIR)/scanners.c $(SRCDIR)/html/*.o
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -f spec.md fuzz.txt spec.html
