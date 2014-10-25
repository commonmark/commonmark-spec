CommonMark
==========

CommonMark is a [specification of Markdown syntax][the spec],
together with BSD3-licensed implementations in C and JavaScript.

The implementations
-------------------

The C implementation provides both a library and a standalone program
`cmark` that converts Markdown to HTML.  It is written in standard C99
and has no library dependencies.  (However, if you check it out from the
repository, you'll need [`re2c`](http://re2c.org) to generate
`scanners.c` from `scanners.re`.  This is only a build dependency for
developers, since `scanners.c` can be provided in a released source
tarball.)

    Usage:   cmark [FILE*]
    Options: --help, -h    Print usage information
             --ast         Print AST instead of HTML
             --version     Print version

The JavaScript implementation is a single JavaScript file, with
no dependencies, that can be linked to in an HTML page.  (To build,
it, do `make js/commonmark.js`---this requires `browserify`, which you
can get using `npm install -g browserify`.) A command-line
version (using `node.js`) is also provided (`js/bin/commonmark`), and
there is a "dingus" for playing with it interactively.  (`make dingus`
will start this.)

[Try it now!](http://jgm.github.io/stmd/js/)

Note that neither implementation attempts to sanitize link attributes or
raw HTML.  If you use these libraries in applications that accept
untrusted user input, you must run the output through an HTML
sanitizer to protect against
[XSS attacks](http://en.wikipedia.org/wiki/Cross-site_scripting).

[The spec] contains over 450 embedded examples which serve as conformance
tests.  To run the tests for `cmark`, do `make test`.  To run them for
another Markdown program, say `myprog`, do `make test PROG=myprog`.  To
run the tests for `commonmark.js`, do `make testjs`.

[The spec]:  http://jgm.github.io/stmd/spec.html

The spec
--------

The source of [the spec] is `spec.txt`.  This is basically a Markdown
file, with code examples written in a shorthand form:

    .
    Markdown source
    .
    expected HTML output
    .

To build an HTML version of the spec, do `make spec.html`.  To build a
PDF version, do `make spec.pdf`.  Both these commands require that
pandoc is installed, and creating a PDF requires a latex installation.

The spec is written from the point of view of the human writer, not
the computer reader.  It is not an algorithm---an English translation of
a computer program---but a declarative description of what counts as a block
quote, a code block, and each of the other structural elements that can
make up a Markdown document.

Because John Gruber's [canonical syntax
description](http://daringfireball.net/projects/markdown/syntax) leaves
many aspects of the syntax undetermined, writing a precise spec requires
making a large number of decisions, many of them somewhat arbitrary.
In making them, I have appealed to existing conventions and
considerations of simplicity, readability, expressive power, and
consistency.  I have tried to ensure that "normal" documents in the many
incompatible existing implementations of Markdown will render, as far as
possible, as their authors intended.  And I have tried to make the rules
for different elements work together harmoniously.  In places where
different decisions could have been made (for example, the rules
governing list indentation), I have explained the rationale for
my choices.  In a few cases, I have departed slightly from the canonical
syntax description, in ways that I think further the goals of Markdown
as stated in that description.

For the most part, I have limited myself to the basic elements
described in Gruber's canonical syntax description, eschewing extensions
like footnotes and definition lists.  It is important to get the core
right before considering such things. However, I have included a visible
syntax for line breaks and fenced code blocks.

In all of this, I have been guided by eight years experience writing
Markdown implementations in several languages, including the first
Markdown parser not based on regular expression substitutions
([pandoc](http://github.com/jgm/pandoc)) and the first markdown parsers
based on PEG grammars
([peg-markdown](http://github.com/jgm/peg-markdown),
[lunamark](http://github.com/jgm/lunamark)). Maintaining these projects
and responding to years of user feedback have given me a good sense of
the complexities involved in parsing Markdown, and of the various design
decisions that can be made.  I have also explored differences between
Markdown implementations extensively using [babelmark
2](http://johnmacfarlane.net/babelmark2/).  In the early phases of
working out the spec, I benefited greatly from collaboration with David
Greenspan, and from feedback from several industrial users of Markdown,
including Jeff Atwood, Vincent Marti, and Neil Williams.
