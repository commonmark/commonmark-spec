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
FUZZCHARS?=2000000  # for fuzztest
BENCHDIR=bench
BENCHFILE=$(BENCHDIR)/benchinput.md
ALLTESTS=alltests.md
NUMRUNS?=10
PROG?=$(BUILDDIR)/src/cmark
BENCHINP?=README.md
JSMODULES=$(wildcard js/lib/*.js)
VERSION?=$(SPECVERSION)
RELEASE?=CommonMark-$(VERSION)

.PHONY: all spec leakcheck clean fuzztest dingus upload jshint test testjs benchjs update-site upload-site check npm debug mingw archive tarball ziparchive testtarball testziparchive testlib bench astyle

all: $(PROG) man/man3/cmark.3
	@echo "Binaries can be found in $(BUILDDIR)/src"

$(PROG): $(BUILDDIR)
	@make -j2 -C $(BUILDDIR)

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

man/man3/cmark.3: src/cmark.h | $(PROG)
	python3 man/make_man_page.py $< > $@ \

archive:
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).tar.gz HEAD
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).zip HEAD

clean:
	rm -rf $(BUILDDIR) $(MINGW_BUILDDIR) $(MINGW_INSTALLDIR) $(PKGDIR)

# We include html_unescape.h in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/html_unescape.h: $(SRCDIR)/html_unescape.gperf
	gperf -L ANSI-C -I -t -N find_entity -H hash_entity -K entity -C -l \
		-F ',{0}' --null-strings -m5 -P -Q entity_pool $< > $@

# We include case_fold_switch.inc in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl tools/mkcasefold.pl < $< > $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	re2c --case-insensitive -b -i --no-generation-date -o $@ $<

test: $(SPEC) $(BUILDDIR)
	make -C $(BUILDDIR) test || (cat $(BUILDDIR)/Testing/Temporary/LastTest.log && exit 1)

$(ALLTESTS): spec.txt
	python3 test/spec_tests.py --spec $< --dump-tests | python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); print("\n".join([test["markdown"] for test in tests]))' > $@

leakcheck: $(ALLTESTS)
	cat $< | valgrind --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) >/dev/null
	cat $< | valgrind --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) -t man >/dev/null
	cat $< | valgrind --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) -t xml >/dev/null

fuzztest:
	{ for i in `seq 1 10`; do \
	  cat /dev/urandom | head -c $(FUZZCHARS) | iconv -f latin1 -t utf-8 | tee fuzz-$$i.txt | \
		/usr/bin/env time -p $(PROG) >/dev/null && rm fuzz-$$i.txt ; \
	done } 2>&1 | grep 'user\|abnormally'

progit:
	git clone https://github.com/progit/progit.git

$(BENCHFILE): progit
	echo "" > $@
	for lang in ar az be ca cs de en eo es es-ni fa fi fr hi hu id it ja ko mk nl no-nb pl pt-br ro ru sr th tr uk vi zh zh-tw; do \
		cat progit/$$lang/*/*.markdown >> $@; \
	done

bench: $(BENCHFILE)
	{ sudo renice 99 $$$$; \
	  for x in `seq 1 $(NUMRUNS)` ; do \
	  /usr/bin/env time -p $(PROG) </dev/null >/dev/null ; \
	  /usr/bin/env time -p $(PROG) $< >/dev/null ; \
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
	eslint -c js/eslint.json ${JSMODULES} js/bin/commonmark js/test.js js/bench.js

benchjs:
	sudo renice 99 $$$$; node js/bench.js ${BENCHINP}

npm:
	cd js; npm publish

dingus: js/commonmark.js
	echo "Starting dingus server at http://localhost:9000/dingus.html" && python -m SimpleHTTPServer 9000

### Spec ###

spec.md: $(SPEC)
	python3 tools/makespec.py markdown > $@

spec.html: spec.txt tools/template.html ${PROG}
	python3 tools/makespec.py html > $@

spec.pdf: spec.md tools/template.tex tools/specfilter.hs
	pandoc -s $< --template tools/template.tex \
	   --filter tools/specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides

### Website ###

update-site: spec js/commonmark.js
	make -C $(SITE) update

upload-site: spec js/commonmark.js
	make -C $(SITE) upload
