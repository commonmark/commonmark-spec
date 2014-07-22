Standard markdown
=================

Standard markdown is a specification of markdown syntax, together
with implementations (`stmd`) in C and javascript.

The C implementation provides both a library and a standalone program
that converts markdown to HTML.  It is written in standard C99 and has
no library dependencies.  (However, if you check it out from the
repository, you'll need `re2c` to generate `scanners.c` from
`scanners.re`.  This is only a build dependency for developers, since
`scanners.c` can be provided in a released source tarball.)

The javascript implementation is a single javascript file
that can be linked to an HTML page.  A standalone version (using
`node.js`) is also provided (`js/markdown`), and there is a
"dingus" for playing with it interactively.  (`make dingus` will start
this.)

The spec contains over 400 embedded examples which serve as
conformance tests.  To run the tests for `stmd`, do `make test`.
To run them for another markdown program, say `myprog`,
do `make test PROG=myprog`.  To run the tests for `stmd.js`,
do `make testjs`.

The source of the spec is `spec.txt`.  This is basically a markdown
file, with code examples written in a shorthand form:

    .
    markdown source
    .
    expected HTML output
    .

To build an HTML version of the spec, do `make spec.html`.
To build a PDF version, do `make spec.pdf`.  Both these commands
require that pandoc is installed, and creating a PDF requires
a latex installation.

