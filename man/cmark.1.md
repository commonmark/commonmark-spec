---
title: cmark
section: 1
footer: cmark manual
date: October 22, 2014
...

# NAME

cmark - convert CommonMark formatted text to HTML

# SYNOPSIS

cmark [*options*] [file\*]

# DESCRIPTION

`cmark` acts as a pipe, reading from stdin or from the specified
files and writing to stdout.  It converts Markdown formatted plain
text to HTML, using the conventions described in the CommonMark
spec.

# OPTIONS

`--ast`
:   Print an abstract syntax tree instead of HTML.

`--help`
:   Print usage information.

`--version`
:   Print version.

# AUTHORS

John MacFarlane
