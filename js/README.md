CommonMark
==========

CommonMark is a rationalized version of Markdown syntax,
with a [spec][the spec] and BSD3-licensed reference
implementations in C and JavaScript.

  [the spec]: http://spec.commonmark.org

For more information, see <http://commonmark.org>.

To play with this library without installing it, see
the live dingus at <http://spec.commonmark.org/dingus.html>.

Installing
----------

You can install the library using `npm`:

    npm install commonmark

This package includes the commonmark library and a
command-line executable, `commonmark`.

For client-side use, you can do `make browserify` to produce
a standalone JavaScript file `js/commonmark.js`,
suitable for linking into a web page, or just fetch
<http://spec.commonmark.org/js/commonmark.js>.

Usage
-----

Instead of converting Markdown directly to HTML, as most converters
do, `commonmark.js` parses Markdown to an AST (abstract syntax tree),
and then renders this AST as HTML.  This opens up the possibility of
manipulating the AST between parsing and rendering.  For example, one
could transform all emphasis into ALL CAPS.

Here's a basic usage example:

    var reader = new commonmark.DocParser();
    var writer = new commonmark.HtmlRenderer();
    var parsed = reader.parse("Hello *world*"); // parsed is a 'Node' tree
    var result = writer.render(parsed);  // result is a string

<!-- TODO

- example of tree manipulation
- options
- API documentation (each function)

-->
