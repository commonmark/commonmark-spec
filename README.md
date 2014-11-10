CommonMark
==========

CommonMark is a rationalized version of Markdown syntax,
with a [spec][the spec] and BSD3-licensed reference
implementations in C and JavaScript.

The implementations
-------------------

The C implementation provides both a library and a standalone program
`cmark` that converts CommonMark to HTML.  It is written in standard C99
and has no library dependencies.  (However, if you check it out from the
repository, you'll need [`re2c`](http://re2c.org) to generate
`scanners.c` from `scanners.re`.  This is only a build dependency for
developers, since `scanners.c` can be provided in a released source
tarball.)

The parser is very fast, on par with
[sundown](https://github.com/vmg/sundown).  Some benchmarks (on
an ancient Thinkpad running Intel Core 2 Duo at 2GHz, measured using
`time` and parsing a
~500K book, the English version of [*Pro
Git*](https://github.com/progit/progit/tree/master/en) by
Scott Chacon and Ben Straub):

|Implementation | Time  |  Factor|
|---------------|-------|--------|
| Markdown.pl   | 5.162s|   286.8|
| PHP Markdown  | 1.021s|    56.7|
| commonmark.js | 0.292s|    16.2|
| peg-markdown  | 0.279s|    15.5|
| marked        | 0.239s|    13.3|
| discount      | 0.090s|     5.0|
| **cmark**     | 0.020s|     1.1|
| sundown       | 0.018s|     1.0|

    Usage:   cmark [FILE*]
    Options: --help, -h    Print usage information
             --ast         Print AST instead of HTML
             --version     Print version

The JavaScript implementation is a single JavaScript file, with
no dependencies, that can be linked to in an HTML page.  To build,
it, do `make js/commonmark.js` (this requires `browserify`, which you
can get using `npm install -g browserify`).  You can also fetch
a pre-built copy from `http://spec.commonmark.org/js/commonmark.js`.
A command-line version (using `node.js`) is also provided
(`js/bin/commonmark`), and there is a "dingus" for playing with it
interactively.  (`make dingus` will start this.)

[Try it now!](http://jgm.github.io/CommonMark/js/)

**Note:** neither implementation attempts to sanitize link attributes or
raw HTML.  If you use these libraries in applications that accept
untrusted user input, you must run the output through an HTML
sanitizer to protect against
[XSS attacks](http://en.wikipedia.org/wiki/Cross-site_scripting).

[The spec] contains over 500 embedded examples which serve as conformance
tests.  To run the tests for `cmark`, do `make test`.  To run them for
another Markdown program, say `myprog`, do `make test PROG=myprog`.  To
run the tests for `commonmark.js`, do `make testjs`.

[The spec]:  http://jgm.github.io/CommonMark/spec.html

Installing
----------

To install the C program (and shared library), [cmake] is required:

    mkdir build
    cd build
    cmake ..     # optionally: -DCMAKE_INSTALL_PREFIX=path
    make         # executable will be created as build/src/cmake
    make install

To run tests:

    perl runtests.pl spec.txt build/src/cmark

To build the javascript library:

    browserify --standalone commonmark js/lib/index.js -o js/commonmark.js

To run tests:

    node js/test.js

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

There are only a few places where this spec says things that contradict
the canonical syntax description:

-   It [allows all punctuation symbols to be
    backslash-escaped](http://jgm.github.io/stmd/spec.html#backslash-escapes),
    not just the symbols with special meanings in Markdown. I found
    that it was just too hard to remember which symbols could be
    escaped.

-   It introduces an [alternative syntax for hard line
    breaks](http://jgm.github.io/stmd/spec.html#hard-line-breaks), a
    backslash at the end of the line, supplementing the
    two-spaces-at-the-end-of-line rule. This is motivated by persistent
    complaints about the “invisible” nature of the two-space rule.

-   Link syntax has been made a bit more predictable (in a
    backwards-compatible way). For example, `Markdown.pl` allows single
    quotes around a title in inline links, but not in reference links.
    This kind of difference is really hard for users to remember, so the
    spec [allows single quotes in both
    contexts](http://jgm.github.io/stmd/spec.html#links).

-   The rule for HTML blocks differs, though in most real cases it
    shouldn't make a difference. (See
    [here](http://jgm.github.io/stmd/spec.html#html-blocks) for
    details.) The spec's proposal makes it easy to include Markdown
    inside HTML block-level tags, if you want to, but also allows you to
    exclude this. It is also makes parsing much easier, avoiding
    expensive backtracking.

-   It does not collapse adjacent bird-track blocks into a single
    blockquote:

        > this is two

        > blockquotes

        > this is a single
        >
        > blockquote with two paragraphs

-   Rules for content in lists differ in a few respects, though (as with
    HTML blocks), most lists in existing documents should render as
    intended. There is some discussion of the choice points and
    differences [here](http://jgm.github.io/stmd/spec.html#motivation).
    I think that the spec's proposal does better than any existing
    implementation in rendering lists the way a human writer or reader
    would intuitively understand them. (I could give numerous examples
    of perfectly natural looking lists that nearly every existing
    implementation flubs up.)

-   The spec stipulates that two blank lines break out of all list
    contexts.  This is an attempt to deal with issues that often come up
    when someone wants to have two adjacent lists, or a list followed by
    an indented code block.

-   Changing bullet characters, or changing from bullets to numbers or
    vice versa, starts a new list. I think that is almost always going
    to be the writer's intent.

-   The number that begins an ordered list item may be followed by
    either `.` or `)`. Changing the delimiter style starts a new
    list.

-   The start number of an ordered list is significant.

-   [Fenced code blocks](http://jgm.github.io/stmd/spec.html#fenced-code-blocks) are supported, delimited by either
    backticks (` ``` `) or tildes (` ~~~ `).

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

Contributing
------------

There is a [forum for discussing
CommonMark](http://talk.commonmark.org); you should use it instead of
github issues for questions and possibly open-ended discussions.
Use the [github issue tracker](http://github.com/jgm/stmd/issues)
only for simple, clear, actionable issues.

[cmake]: http://www.cmake.org/download/

