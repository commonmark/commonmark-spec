---
title: Linguistic extensions to CommonMark
subtitle: Documentation and embedded conformance tests
version: 0.1
date: '2018-12-01'
license: '[CC0](http://creativecommons.org/licenses/cc0/)'
author: Christoph Päper
alternative: 
---

Introduction
------------

Modules
-------

Requirements
------------

* Prettify:

   * An asterisk directly preceding a word `*wrong` doesn’t start emphasis: *_wrong_, affected paragraph or span should get an appropriate class for styling
   * A question mark directly preceding a word `?doubtful` is put in superscript: <sup>?</sup>_doubtful_, affected paragraph or span should get an appropriate class for styling
   * Less-than `<` and greater-than `>` marks *surrounded by spaces* show etymology and may be replaced by arrows &larr;, &rarr; or other prettier symbols `≺ ≻`.

* strings enclosed in phonemic `/slashes/` or phonetic `[square brackets]` use (Z-/X-)SAMPA or Kirshenbaum and are converted to Unicode IPA, might prefer a sans-serif typeface
   * except ones preceded by plus `+` or minus `-` sign for `[±features]`
   * except all-uppercase `[MORPH(EME)S]`, also see `{morph(eme)}`
   * double slashes `//phonemic//` or brackets `[[phonetic]]` would be tolerable
   * since Markdown uses square brackets for (shortcut reference) links, another syntax like `/[phonetic]/` or `[/phonetic/]` might also be tolerable
   * `/transliteration/` with ditched slashes might be interesting.
   * **Conclusion:**
     1. transliterate anything between two slashes (outside URLs and code) into Unicode IPA, remove the slashes
     2. Use double slashes for phonemic `//foo//` and slashes within square brackets for phonetic `[/foo/]` notation

* strings in `<angular brackets>` are 〈graphemic〉 unless they can be parsed as an absolute URL or HTML tag. 

   * Since stuff like `<a>` and `<b>` would be HTML, use double brackets `<<foo>>`.
   * Might be used for transliteration from or to romanized text.

* Digits, possibly followed by apostrophes `'` (or other symbols `"`), inside round parentheses ``(1)`, `(2')` mark examples at the beginning of a line and references to them elsewhere. 

   * Double parentheses for references `((3))` would be tolerable.
   * [Numbered examples extension](example)

* Phrases enclosed by single quotation marks `'`, `‘`, `’` including accent marks `´` are set in italics and are not emphasized (HTML `<i>`, not `<em>`). Alternatively, distinguish underscore `_` (`<i>`, `__` `<b>`) and asterisk `*` (`<em>`, `**` `<strong>`) emphasis.

* Inside regular quotations `> foo`, `“foo”` or`"foo"`, round and square brackets are highlighted, especially if they only contain dots `[...]`, `(…)` or uppercase letters `[SIC]`, `[sic!]`.

* Some syntax for simple trees.

   ```` markdown
   /   | root |   \
   | 1 |  2   | 3 |
   
   |   | root |   |
     /    |     \  
   | 1 |  2   | 3 |
   
   |   | root |   |
   /   /      \   \
   | 1 |  2   | 3 |
   
   
    |   |root|   |
    /   /    \   \
   | 1 |  2   | 3 |
   ````

   * embed DOT / GraphViz

     ~~~~ markdown
     ```` graphviz dot
     digraph {
         root -> {1 2 3};
     }
     ````
     ~~~~

   * embed Mermaid

     ~~~~ markdown
     ```` mermaid
     graph foo;
         root-->1;
         root-->2;
         root-->3;
     ````
     ~~~~

* [Pipe table syntax](basic-pipe-table) (without header) reused for interlinear glosses.

   ```` markdown
   |  1  |  2  |   3   |
   | one | two | three |
   ````

* Other markup, like `|letter|`, could be added easily.

https://linguistics.stackexchange.com/questions/7240/markdown-for-linguistics

- [extension](file name without extension), notes
