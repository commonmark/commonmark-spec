---
title: CommonMark
...

CommonMark is a [specification of Markdown
syntax](http://jgm.github.io/stmd/spec.html), together with
BSD3-licensed implementations (`cmark`) in C and JavaScript. The source
for the spec and the two implementations can be found in [this
repository](http://github.com/jgm/stmd).

The C implementation provides both a library and a standalone program
`cmark` that converts Markdown to HTML. It is written in standard C99
and has no library dependencies.

The JavaScript implementation is a single JavaScript file, with no
dependencies. [Try it now!](http://jgm.github.io/stmd/js/)

[The spec](http://jgm.github.io/stmd/spec.html) contains over 400
embedded examples which serve as conformance tests. (The source contains
a perl script that will run the tests against any Markdown program.)

The spec is written from the point of view of the human writer, not the
computer reader. It is not an algorithm—an English translation of a
computer program—but a declarative description of what counts as a block
quote, a code block, and each of the other structural elements that can
make up a Markdown document. For the most part, the spec limits itself
to the basic elements described in John Gruber’s [canonical syntax
description](http://daringfireball.net/projects/markdown/syntax),
eschewing extensions like footnotes and definition lists. It is
important to get the core right before considering such things.

Because Gruber’s syntax description leaves many aspects of the syntax
undetermined, writing a precise spec requires making a large number of
decisions, many of them somewhat arbitrary. In making them, I have
appealed to existing conventions and considerations of simplicity,
readability, expressive power, and consistency. I have tried to ensure
that “normal” documents in the many incompatible existing
implementations of Markdown will render, as far as possible, as their
authors intended. And I have tried to make the rules for different
elements work together harmoniously. In places where different decisions
could have been made (for example, the rules governing list
indentation), I have explained the rationale for my choices. In a few
cases, I have departed slightly from the canonical syntax description,
in ways that I think further the goals of Markdown as stated in that
description.

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
([pandoc](http://github.com/jgm/pandoc)) and the first Markdown parsers
based on PEG grammars
([peg-markdown](http://github.com/jgm/peg-markdown),
[lunamark](http://github.com/jgm/lunamark)). Maintaining these projects
and responding to years of user feedback have given me a good sense of
the complexities involved in parsing Markdown, and of the various design
decisions that can be made. I have also explored differences between
Markdown implementations extensively using [babelmark
2](http://johnmacfarlane.net/babelmark2/). In the early phases of
working out the spec, I benefited greatly from collaboration with David
Greenspan, and from extensive discussions with a group of industrial
users of Markdown, including Jeff Atwood, Vincent Marti, and Neil
Williams.

### Contributing

There is a [forum for discussing
CommonMark](http://talk.commonmark.org); you should use it instead of
github issues for questions and possibly open-ended discussions.
Use the [github issue tracker](http://github.com/jgm/stmd/issues)
only for simple, clear, actionable issues.

