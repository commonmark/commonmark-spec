---
title: Markua extensions to CommonMark
subtitle: Documentation and embedded conformance tests
version: 0.1
date: '2018-12-01'
license: '[CC0](http://creativecommons.org/licenses/cc0/)'
alternative: https://leanpub.com/markua/read
---

Leanpub's _Markua_ supports a subset of Markdown with extensions. It limits each syntax choice Commonmark offers to a single one. This flavor description only documents the extensions and only those compatible with Commonmark.

- [Presentational Emphasis extension](file name without extension), single underscore italic `_i_`, double underscore bold `__b__`
- [Underlined Text extension](underline), quadruple underscore `____foo____`
- [Strike-Through extension](strikethrough), double tilde `~~foo~~`
- [Superscript extension](superscript), suffix required, single-character `^` 
- [Figures extension](figure), not restricted to images
- [Definition Lists extension](definition-list), `foo\n: bar`
- [Footnotes and Endnotes extension](footnote), `[^footnote]` vs. `[^^endnote]`
- [Explicit Targets extension](explicit-target)
- [Emoji Short Code extension](shortcode), `:smile:`
- [Critic Markup](critic-markup)

## Missing Extensions

- Markua distinguishes prefixed headings from affixed headings (which do contain a suffix), at least for the to level:
  `# Part #` vs. `# Chapter`
- Executed code blocks, superset of the [Diagrams extension](diagram).
- alphabetized lists
- roman-numbered lists
- page break with `===`, cf. [Sections extension](section)
- _Smarty Pants_, prettify punctuation marks like dashes `--`, `\w--\w`, `--\w`, `---`, `\w-\w`, ellipses `...`, quotation marks `\w""\w`, `\w''\w` and spostrophes `'\w`, as well as emoticons `:-)`, `;)` etc. ; soft hyphen `\-`, Unicode `\u0123`

Unsupported or Incompatible Extensions
----------------------

This repository does not document extensions that use fixed (English) keywords instead of punctuation characters for markup.

- predefined keys for attributes
- asides and blurbs
  - `A>` and `B>`, also `D>`, `E>`, `I>`, `Q>`, `T>`, `W>`, `X>`
  - `{aside}` and `{blurb}`
- Crosslink variables: `[title #t, number #n, description #d](#foo)`, `[full title #f](#foo)`
- [Quizzes and Exercises extension](quiz), `\n?  Question`, `! correct free answer`, uppercase list letter for correct multiple choise ansers
- Index entries `{i: "foo"}`
- [Document settings](document-metadata) uses curly braces `{` and `}` as delimiters for what is basically YAML but not JSON data

