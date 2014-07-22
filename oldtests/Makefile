SHELL=/bin/bash
TESTDIR ?= *
PATT ?= .
TESTS=$(shell ls $(TESTDIR)/*.markdown | grep $(PATT))
DIFFS=$(patsubst %.markdown,%.diff,$(TESTS))
PROG ?= ../stmd
FILTER ?= perl -pe 's/ /␣/g'
TIDYCMD ?= tidy -asxhtml -utf8 --show-body-only yes --show-warnings no -quiet
DETAILS ?= 1

# Check to see if echo supports -e option to allow backslash escapes
ifeq ($(shell echo -e),-e)
ECHO=echo
else
ECHO=echo -e
endif

all: $(DIFFS)
	PASS=0;TESTS=0; \
	for f in $(DIFFS); do \
	  let TESTS=TESTS+1; \
	  [ -s $$f ] || let PASS=PASS+1; \
	done; \
	$(ECHO)  "\033[1m$$PASS of $$TESTS tests passed.\033[0m"; \
	if [ $$TESTS -eq $$PASS ]; then exit 0; else exit 1; fi

%.actual.html: %.markdown
ifeq ($(TIDY),1)
	-cat $< | $(PROG) | $(TIDYCMD) > $@
else
	-cat $< | $(PROG) > $@
endif

%.expected.html: %.html
ifeq ($(TIDY),1)
	-$(TIDYCMD) $< > $@
else
	cp $< $@
endif

%.diff: %.expected.html %.actual.html
	diff --unified=1 <(cat $(word 1,$^) | $(FILTER)) <(cat $(word 2,$^) | $(FILTER)) > $@ ; \
	  if [ -s $@ ]; then \
		$(ECHO)  "\033[1;31m✘ $(patsubst %.diff,%,$@)\033[0m"; \
		if [ $(DETAILS) == "1" ]; then \
			$(ECHO)  "\033[0;36m" ; cat $@; $(ECHO)  "\033[0m"; \
		fi \
	  else \
		$(ECHO)  "\033[1;32m✓ $(patsubst %.diff,%,$@)\033[0m"; \
	  fi

.PHONY: all clean

clean:
	-@rm */*.{diff,actual.html,expected.html}
