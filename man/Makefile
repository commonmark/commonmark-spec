ALL=man1/cmark.1

.PHONY: all

all: $(ALL)
man1/%.1: %.1.md
	pandoc $< -o $@ -s -t man
