CommonMark
==========

CommonMark is a rationalized version of Markdown syntax,
with a [spec][the spec] and BSD3-licensed reference
implementations in C and JavaScript.

For more information, see <http://commonmark.org>.

To play with this library without installing it, see
the live dingus at <http://spec.commonmark.org/dingus.html>.

This package includes the commonmark library and a
command-line executable, `commonmark`.

Basic usage example:

    var reader = new commonmark.DocParser();
    var writer = new commonmark.HtmlRenderer();
    var parsed = reader.parse("Hello *world*");
    var result = writer.render(parsed);

 [the spec]: http://spec.commonmark.org

