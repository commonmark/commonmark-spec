---
title: GitLab extensions to CommonMark
subtitle: Documentation and embedded conformance tests
version: 0.1
date: '2018-10-01'
license: '[CC-BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/)'
alternative: https://gitlab.com/gitlab-org/gitlab-ce/blob/master/doc/user/markdown.md
---

GitLab traditionally supported Markdown with tables, strike-through, links from plain URLs and for `@` mentions and `#` issue numbers and some other extensions. 
Their syntax variant is commonly known as _GitLab-flavored Markdown_, abbreviated as GFM which is shared by _GitHub-flavored Markdown_. 
When the site switched to a CommonMark-compliant parser in late-2018, GitLab kept these extensions that were mostly inherited from **Redcarpet**.

- [basic pipe tables](basic-pipe-table), excluding MMD/PME extended pipe table features
- [strike through](strikethrough), fixed to two tildes on both sides
- [plain links](plain-link), absolute URLs and email addresses, but also including account mentions and issue numbers as well as a number of other prefixes
- [to-do lists](todo-list), on top of bullet lists
- [footnotes](footnote), alphabetic and numeric, not enabled on github.com
- [emoji shortcode](shortcode), part of later parsing step after `gfm-cmark`
- [HTML element type filter](html-filter)
