SUBDIRS=src

test: spec.txt
	$(PERL) runtests.pl $< $(abs_srcdir)/src/cm

spec: test spec.html

spec.md: spec.txt
	$(PERL) $(abs_srcdir)/spec2md.pl < $< > $@

spec.html: spec.md template.html
	$(PANDOC) --no-highlight --number-sections --template $(abs_srcdir)/template.html -s --toc -S $< > $@ # | perl -pe 's/␣/<span class="space"> <\/span>/g' > $@

narrative.html: narrative.md template.html
	$(PANDOC) --template template.html -s -S $< -o $@

spec.pdf: spec.md template.tex specfilter.hs
	$(PANDOC) -s $< --template template.tex \
	   --filter ./specfilter.hs -o $@ --latex-engine=xelatex --toc \
	   --number-sections -V documentclass=report -V tocdepth=2 \
	   -V classoption=twosides
