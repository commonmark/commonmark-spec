SRCDIR?=src
DATADIR?=data
BENCHINP?=README.md
JSMODULES=$(wildcard js/lib/*.js)
SPEC=spec.txt
SITE=_site
PKGDIR=cmark-$(SPECVERSION)
BUILDDIR=build
FUZZCHARS?=2000000  # for fuzztest
PROG?=$(BUILDDIR)/src/cmark
SPECVERSION=$(shell grep version: $(SPEC) | sed -e 's/version: *//')

.PHONY: all spec leakcheck clean fuzztest dingus upload jshint test testjs benchjs update-site upload-site check npm debug tarball

all: $(BUILDDIR)
	@cmake --build $(BUILDDIR) || (echo "You need cmake to build this program: http://www.cmake.org/download/" && exit 1)

install: $(BUILDDIR) man/man1/cmark.1
	cmake --build $(BUILDDIR) --target install

$(BUILDDIR):
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

debug:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Debug; \
	cmake --build .

tarball: spec.html
	rm -rf $(PKGDIR); \
	mkdir -p $(PKGDIR)/man/man1; \
	cp -r src $(PKGDIR)/; \
	cp spec.html $(PKGDIR); \
	cp CMakeLists.txt $(PKGDIR); \
	perl -ne '$$p++ if /^### JavaScript/; print if (!$$p)' Makefile > $(PKGDIR)/Makefile; \
	cp man/man1/cmark.1 $(PKGDIR)/man/man1/; \
	cp README.md LICENSE spec.txt runtests.pl $(PKGDIR)/; \
	tar czf cmark-$(SPECVERSION).tar.gz $(PKGDIR); \
	rm -rf $(PKGDIR)

clean:
	rm -rf $(BUILDDIR)

$(PROG): all

$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl mkcasefold.pl < $< > $@

man/man1/cmark.1: man/cmark.1.md
	pandoc $< -o $@ -s -t man

test: $(SPEC)
	perl runtests.pl $< $(PROG)

testlib: $(SPEC)
	perl runtests.pl $< ./wrapper.py

leakcheck: $(PROG)
	cat leakcheck.md | valgrind --leak-check=full --dsymutil=yes $(PROG)

fuzztest:
	{ for i in `seq 1 10`; do \
	  cat /dev/urandom | head -c $(FUZZCHARS) | iconv -f latin1 -t utf-8 | tee fuzz-$$i.txt | \
		/usr/bin/env time -p $(PROG) >/dev/null && rm fuzz-$$i.txt ; \
	done } 2>&1 | grep 'user\|abnormally'

operf: $(PROG)
	operf $(PROG) <$(BENCHINP) >/dev/null

distclean: clean
	-rm -f js/commonmark.js
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -f spec.md fuzz.txt spec.html

### JavaScript ###

js/commonmark.js: js/lib/index.js ${JSMODULES}
	browserify --standalone commonmark $< -o $@

testjs: $(SPEC)
	node js/test.js

jshint:
	jshint ${JSMODULES}

benchjs:
	node js/bench.js ${BENCHINP}

npm:
	cd js; npm publish

dingus: js/commonmark.js
	echo "Starting dingus server at http://localhost:9000" && python -m SimpleHTTPServer 9000

### Spec ###

spec.md: $(SPEC)
	perl spec2md.pl < $< > $@

spec.html: spec.md template.html
	pandoc --no-highlight --number-sections --template template.html -s --toc -S $< | \
	perl -pe 's/a href="@([^"]*)"/a id="\1" href="#\1" class="definition"/g' | \
	perl -pe 's/␣/<span class="space"> <\/span>/g' \
	> $@

spec.pdf: spec.md template.tex specfilter.hs
	pandoc -s $< --template template.tex \
	   --filter ./specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides

### Website ###

update-site: spec.html js/commonmark.js
	make -C $(SITE) update

upload-site: spec.html
	make -C $(SITE) upload
