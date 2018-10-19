---
title: GitHub extensions to CommonMark
subtitle: Documentation and embedded conformance tests
version: 0.2
date: '2018-02-01'
license: '[CC-BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/)'
alternative:
- https://github.com/github/cmark/blob/master/test/extensions.txt 
- https://github.github.com/gfm/
---

GitHub traditionally supported Markdown with tables, strike-through, links from plain URLs and for `@` mentions and `#` issue numbers. 
Their syntax variant is commonly known as _GitHub-flavored Markdown_ or GFM. 
When the site switched to a CommonMark-compliant parser in mid-2017, GitHub kept these extensions.

- [basic pipe tables](basic-pipe-table), excluding MMD/PME extended pipe table features
- [strike through](strikethrough)
- [plain links](plain-link), absolute URLs and email addresses, but also including account mentions and issue numbers
- [to-do lists](todo-list), on top of bullet lists
- [footnotes](footnote), alphabetic and numeric, not enabled on github.com
- [emoji shortcode](shortcode), part of later parsing step after `gfm-cmark`
- [HTML element type filter](html-filter)
