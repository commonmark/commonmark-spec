# Desirable and popular Commonmark extensions

## Single-link paragraphs

### [Figures with images](figure)

~~~~ markdown
![figure caption](figure-source.ext)

![figure text](figure-source.ext "figure caption")

![figure caption][figure label]

![figure text][]

 [figure label]: figure-source.ext
 [figure text]: figure-source.ext "figure caption"

[![figure caption](figure-source.ext)](http://example.org)
~~~~

### [Multimedia](multimedia)

~~~~ markdown
![audio title](audio-source.mp3)

![video title](video-source.mp4)
~~~~

### Link previews

Metadata (`<title/>`, favicon), Twitter Card, Open Graph, Yandex Tableau etc.

~~~~ markdown
<http://example.org>

[text](target)
~~~~

## [Footnotes](footnote), Endnotes and Sidenotes

~~~~ markdown
text[^1]
text[^label]

  [^1]: content
  [^label]: content
~~~~

## Text

~~~~ markdown
~~strike~~	<del/>, <s/>
~subscript~	<sub/>	H~2O
^superscript^	<sup/>	m^2
^^insert^^	<ins/>
____underline____
==marked==
:emoji: :text-macro:
~~~~

Prefix only:

~~~~ markdown
@mention
#tag
#123
~~~~

## Attributes

~~~~ markdown
``` "title" 'title' (title) .class #id +on -off key=value @data-custom-key=value {more complex stuff, including line breaks} <source> [] ! ? : = & % $ * _
code
```

[text](<target> "title" .class #id)

## Heading ## #id
~~~~

### Images

~~~~ markdown
![text](source.ext "title" width:height)
![text](source.ext "title" width*height)
![text](source.ext "title" =width:height)
![text](source.ext "title" =width*height)
~~~~

## Lists

~~~~ markdown
Caption for compact list:
- list item
- list item

Definition list:
- definition
Another term:
- definition
- alternate definition

Descending enumerated list
~~~~

## Forms

~~~~ markdown
- [ ] empty checkbox (to do list)
- [x] filled checkbox
- [+] filled checkbox

- ( ) empty select
- (o) chosen select
- (+) chosen select
~~~~

single and multiple select need to be in the same list, other fields can also be outside lists

~~~~ markdown
- [____] text input
- [****] password input
- [####] hidden input
- [____^] file input
- [____#] [######] color input
- [____?] search
- [____!] submit
- [] reset
- [____%] number 
- [____$] 
- [____&] 
- [____,] 
- [____@] email address input
- [____.] [____/] URL input
- button
- [____-] [CCYY-DDD] date
- [____;] datetime
- [____:] [HH:MM:SS] time
- [CCYY-WW] week
- [CCYY-MM] month
- [____-] range
- [____+] tel
~~~~

The list marker can be used to add some kind of state.

~~~~ markdown
+ required?
* optional? random position?
- hidden? fixed position?
~~~~

Other types of common inputs that have standardized codes would be possible. They are mostly considered the domain of microformats:

~~~~ markdown
+ language
+ country, other geographical regions
+ currency
+ airport
+ ISBN, ISSN, ...
+ DOI
+ IBAN, BIC, credit card number
~~~~

## Quotations

~~~~ markdown
> Source:
> Quotation

> Quotation
> -- Source
~~~~


