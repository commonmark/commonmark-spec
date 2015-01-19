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

To run tests for the JavaScript library:

    make testjs

To run benchmarks against some other JavaScript converters:

    npm install showdown marked markdown-it
    make benchjs

To start an interactive dingus that you can use to try out
the library:

    make dingus

Usage
-----

Instead of converting Markdown directly to HTML, as most converters
do, `commonmark.js` parses Markdown to an AST (abstract syntax tree),
and then renders this AST as HTML.  This opens up the possibility of
manipulating the AST between parsing and rendering.  For example, one
could transform emphasis into ALL CAPS.

Here's a basic usage example:

    var reader = new commonmark.Parser();
    var writer = new commonmark.HtmlRenderer();
    var parsed = reader.parse("Hello *world*"); // parsed is a 'Node' tree
    // transform parsed if you like...
    var result = writer.render(parsed);  // result is a string

<!-- TODO

Public API
----------

### Parser

constructor takes options param
explain what can go into options

public
properties: options
methods:  parse

?? should we add a filters [] option?
a filter could be a function that transforms a node,
and the parser could automatically run a walker with
each filter

### Node

getters:  type, firstChild, lastChild,,
  next, prev, parent, sourcepos, isContainer
getters+setters: literal, destination, title,
  info, level, listType, listTight, listStart,
  listDelimiter
methods: appendChild(child),
  prependChild(child),
  unlink(),
  insertAfter(sibling),
  insertBefore(sibling),
  walker()

walker returns NodeWalker object with methods:
  resumeAt(node, entering)
  next() - returns an objcet with properties 'entering' and 'node'

examples:
  capitalize every string
  changing emphasis to ALL CAPS
  de-linkifying
  running all the code samples through a highlighter or other
  transform (svg?)

?? would it be better to include NodeWalker in the API
and have people do walker = new NodeWalker(node)?
probably.

### HtmlRenderer

constructor takes options param
document relevant options

methods:
  escapeXml(string, isAttribute)
  render(node)
properties:
  options


### XmlRenderer

constructor takes options param
document relevant options

methods:
  escapeXml(string, isAttribute)
  render(node)
properties:
  options

-->
