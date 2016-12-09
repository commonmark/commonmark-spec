SITE=_site
SPECVERSION=$(shell perl -ne 'print $$1 if /^version: *([0-9.]+)/' spec.txt)

.PHONY: spec clean

spec: spec.html # spec.pdf spec.md

spec.md: spec.txt tools/template.commonmark
	lua tools/make_spec.lua commonmark < $< > $@

spec.html: spec.txt tools/template.html
	lua tools/make_spec.lua html < $< > $@

spec.tex: spec.txt tools/template.latex
	lua tools/make_spec.lua latex < $< > $@

spec.pdf: spec.tex
	xelatex $<

clean:
	-rm spec.tex spec.md spec.html
