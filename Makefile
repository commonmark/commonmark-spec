SITE=_site
SPECVERSION=$(shell perl -ne 'print $$1 if /^version: *([0-9.]+)/' spec.txt)

.PHONY: all clean npm

all: spec.html # spec.pdf spec.md

spec.md: spec.txt tools/template.commonmark
	lua tools/make_spec.lua commonmark < $< > $@

spec.html: spec.txt tools/template.html
	lua tools/make_spec.lua html < $< > $@

spec.tex: spec.txt tools/template.latex
	lua tools/make_spec.lua latex < $< > $@

spec.pdf: spec.tex
	xelatex $<

npm:
	# Do a sanity check first on versions
	grep -q '"version": *"$(SPECVERSION)' package.json && \
	       npm publish

clean:
	-rm spec.tex spec.md spec.html
