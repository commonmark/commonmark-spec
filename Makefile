SRCDIR?=src
DATADIR?=data
BUILDDIR?=build
GENERATOR?=Unix Makefiles
MINGW_BUILDDIR?=build-mingw
MINGW_INSTALLDIR?=windows
SPEC=spec.txt
SITE=_site
SPECVERSION=$(shell perl -ne 'print $$1 if /^version: *([0-9.]+)/' $(SPEC))
PKGDIR?=cmark-$(SPECVERSION)
TARBALL?=cmark-$(SPECVERSION).tar.gz
ZIPARCHIVE?=cmark-$(SPECVERSION).zip
FUZZCHARS?=2000000  # for fuzztest
BENCHDIR=bench
BENCHFILE=$(BENCHDIR)/benchinput.md
ALLTESTS=alltests.md
NUMRUNS?=10
PROG?=$(BUILDDIR)/src/cmark
BENCHINP?=README.md
JSMODULES=$(wildcard js/lib/*.js)

.PHONY: all spec leakcheck clean fuzztest dingus upload jshint test testjs benchjs update-site upload-site check npm debug mingw archive tarball ziparchive testarchive testtarball testziparchive testlib bench astyle

all: $(PROG)
	@echo "Binaries can be found in $(BUILDDIR)/src"

$(PROG): $(BUILDDIR)
	@make -C $(BUILDDIR)

check:
	@cmake --version > /dev/null || (echo "You need cmake to build this program: http://www.cmake.org/download/" && exit 1)

$(BUILDDIR): check $(SRCDIR)/html_unescape.h $(SRCDIR)/case_fold_switch.inc
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

install: $(BUILDDIR)
	make -C $(BUILDDIR) install

debug:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Debug; \
	make

mingw:
	mkdir -p $(MINGW_BUILDDIR); \
	cd $(MINGW_BUILDDIR); \
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$(MINGW_INSTALLDIR) ;\
	make && make install

archive: spec.html $(BUILDDIR) man/man1/cmark.1 man/make_man_page.py
	@rm -rf $(PKGDIR); \
	mkdir -p $(PKGDIR)/$(SRCDIR); \
	mkdir -p $(PKGDIR)/api_test $(PKGDIR)/man/man1 $(PKGDIR)/man/man3 ; \
	mkdir -p  $(PKGDIR)/test ; \
	srcfiles=`git ls-tree --full-tree -r HEAD --name-only $(SRCDIR) test api_test`; \
	for f in $$srcfiles; do cp -a $$f $(PKGDIR)/$$f; done; \
	cp -a $(SRCDIR)/scanners.c $(PKGDIR)/$(SRCDIR)/; \
	cp -a spec.html $(PKGDIR); \
	cp -a man/CMakeLists.txt $(PKGDIR)/man;\
	cp -a man/make_man_page.py $(PKGDIR)/man;\
	cp -a man/man1/cmark.1 $(PKGDIR)/man/man1;\
	cp CMakeLists.txt $(PKGDIR); \
	perl -ne '$$p++ if /^### JavaScript/; print if (!$$p)' Makefile > $(PKGDIR)/Makefile; \
	cp -a Makefile.nmake nmake.bat $(PKGDIR); \
	cp -a README.md COPYING spec.txt $(PKGDIR)/; \
	tar czf $(TARBALL) $(PKGDIR); \
	zip -q -r $(ZIPARCHIVE) $(PKGDIR); \
	rm -rf $(PKGDIR) ; \
	echo "Created $(TARBALL) and $(ZIPARCHIVE)."

clean:
	rm -rf $(BUILDDIR) $(MINGW_BUILDDIR) $(MINGW_INSTALLDIR) $(TARBALL) $(ZIPARCHIVE) $(PKGDIR)

# We include html_unescape.h in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/html_unescape.h: $(SRCDIR)/html_unescape.gperf
	gperf -L ANSI-C -I -t -N find_entity -H hash_entity -K entity -C -l \
		-F ',0,{0}' --null-strings -m5 $< > $@

# We include case_fold_switch.inc in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl mkcasefold.pl < $< > $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	re2c --case-insensitive -b -i --no-generation-date -o $@ $<

test: $(SPEC) $(BUILDDIR)
	make -C $(BUILDDIR) test ARGS="-V"

$(TARBALL): archive

$(ZIPARCHIVE): archive

testarchive: testtarball testziparchive
	rm -rf $(PKGDIR)

testtarball: $(TARBALL)
	rm -rf $(PKGDIR); \
	tar xvzf $(TARBALL); \
	cd $(PKGDIR); \
	mkdir build && cd build && cmake .. && make && ctest -V

testziparchive: $(ZIPARCHIVE)
	rm -rf $(PKGDIR); \
	unzip $(ZIPARCHIVE); \
	cd $(PKGDIR); \
	mkdir build && cd build && cmake .. && make && ctest -V

$(ALLTESTS): spec.txt
	python3 test/spec_tests.py --spec $< --dump-tests | python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); print("\n".join([test["markdown"] for test in tests]).encode("utf-8"))' > $@

leakcheck: $(ALLTESTS)
	cat $< | valgrind --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) >/dev/null

fuzztest:
	{ for i in `seq 1 10`; do \
	  cat /dev/urandom | head -c $(FUZZCHARS) | iconv -f latin1 -t utf-8 | tee fuzz-$$i.txt | \
		/usr/bin/env time -p $(PROG) >/dev/null && rm fuzz-$$i.txt ; \
	done } 2>&1 | grep 'user\|abnormally'

# for benchmarking
$(BENCHFILE): progit/progit.md
	-rm $@; for x in `seq 1 20` ; do cat $< >> $@; done

progit:
	git clone https://github.com/progit/progit.git

progit/progit.md: progit
	cat progit/en/*/*.markdown > $@

bench: $(BENCHFILE)
	{ sudo renice 99 $$$$; \
	  for x in `seq 1 $(NUMRUNS)` ; do \
	  /usr/bin/env time -p $(PROG) </dev/null >/dev/null ; \
	  /usr/bin/env time -p $(PROG) <$< >/dev/null ; \
		  done \
	} 2>&1  | grep 'real' | awk '{print $$2}' | python3 'bench/stats.py'

astyle:
	astyle --style=linux -t -p -r  'src/*.c' --exclude=scanners.c
	astyle --style=linux -t -p -r  'src/*.h' --exclude=html_unescape.h

operf: $(PROG)
	operf $(PROG) <$(BENCHINP) >/dev/null

distclean: clean
	-rm -f js/commonmark.js
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -f spec.md fuzz.txt spec.html
	-rm -rf $(BENCHFILE) $(ALLTESTS) progit

### JavaScript ###

js/commonmark.js: js/lib/index.js ${JSMODULES}
	browserify --standalone commonmark $< -o $@

testjs: $(SPEC)
	node js/test.js

jshint:
	jshint ${JSMODULES}

lint:
	eslint -c eslint.json ${JSMODULES} js/bin/commonmark js/test.js js/bench.js

benchjs:
	sudo renice 99 $$$$; node js/bench.js ${BENCHINP}

npm:
	cd js; npm publish

dingus: js/commonmark.js
	echo "Starting dingus server at http://localhost:9000" && python -m SimpleHTTPServer 9000

### Spec ###

spec.md: $(SPEC)
	python3 makespec.py markdown > $@

spec.html: spec.txt template.html ${PROG}
	python3 makespec.py html > $@

spec.pdf: spec.md template.tex specfilter.hs
	pandoc -s $< --template template.tex \
	   --filter ./specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides

### Website ###

update-site: spec js/commonmark.js
	make -C $(SITE) update

upload-site: spec js/commonmark.js
	make -C $(SITE) upload
