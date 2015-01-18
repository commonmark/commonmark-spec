# Benchmarks

Some benchmarks, run on an ancient Thinkpad running Intel Core 2 Duo at 2GHz.

|Implementation     |  Time (sec)|
|-------------------|-----------:|
| Markdown.pl       | 2921.24    |
| Python markdown   |  291.25    |
| PHP markdown      |   20.82    |
| kramdown          |   17.32    |
| cheapskate        |    8.24    |
| peg-markdown      |    5.45    |
| parsedown         |    5.06    |
| **commonmark.js** |    2.09    |
| marked            |    1.93    |
| discount          |    1.86    |
| **cmark**         |    0.37    |
| sundown           |    0.34    |


To run these benchmarks, use `make bench PROG=/path/to/program`.

The input text is a 11MB Markdown file built by concatenating the
Markdown sources of all the localizations of the first edition of
[*Pro Git*](https://github.com/progit/progit/tree/master/en) by Scott Chacon.

`time` is used to measure execution speed.  The reported
time is the *difference* between the time to run the program
with the benchmark input and the time to run it with no input.
(This procedure ensures that implementations in dynamic languages are
not penalized by startup time.) Amedian of ten runs is taken.  The
process is reniced to a high priority so that the system doesn't
interrupt runs.

## JavaScript libraries

Here are some focused benchmarks of four JavaScript libraries
(using versions available on 17 Jan 2015). They test performance
on different kinds of Markdown texts.  With the exception of
the first (the `README.md` from this project), the samples are taken
from the [markdown-it](https://github.com/markdown-it/markdown-it)
repository.  Results show a ratio of ops/second (higher is better)
against the slowest implementation (always showdown).

| Sample                 |showdown  |commonmark|marked    |markdown-it|
|------------------------|---------:|---------:|---------:|----------:|
|README.md               |         1|       3.2|       3.1|        4.4|
|block-bq-flat.md        |         1|       8.0|      12.8|       11.3|
|block-bq-nested.md      |         1|      10.8|       9.9|       12.6|
|block-code.md           |         1|      24.9|      58.4|       83.2|
|block-fences.md         |         1|      16.4|      66.5|       65.5|
|block-heading.md        |         1|      10.7|      11.0|       17.5|
|block-hr.md             |         1|      14.4|      14.6|       37.7|
|block-html.md           |         1|       7.9|       2.8|       14.8|
|block-lheading.md       |         1|      14.4|      17.7|       26.1|
|block-list-flat.md      |         1|       4.3|       4.3|       10.4|
|block-list-nested.md    |         1|       7.4|       5.7|       19.1|
|block-ref-flat.md       |         1|       1.8|       1.1|        1.5|
|block-ref-nested.md     |         1|       1.5|       1.4|        2.4|
|inline-autolink.md      |         1|       4.1|       7.0|        4.5|
|inline-backticks.md     |         1|      15.8|      13.7|       30.7|
|inline-em-flat.md       |         1|       4.1|       3.8|        8.7|
|inline-em-nested.md     |         1|       5.2|       4.6|        8.3|
|inline-em-worst.md      |         1|       5.3|       4.9|        3.4|
|inline-entity.md        |         1|       4.8|       9.7|        7.7|
|inline-escape.md        |         1|       4.6|       2.9|       12.4|
|inline-html.md          |         1|       3.6|       5.2|        5.0|
|inline-links-flat.md    |         1|       3.7|       4.0|        4.1|
|inline-links-nested.md  |         1|       3.8|       1.0|        1.5|
|inline-newlines.md      |         1|       6.9|       6.5|       13.1|
|rawtabs.md              |         1|       8.9|       9.7|       13.4|

To run these:

1. `npm install marked showdown markdown-it`
2. Create a `samples` subdirectory and copy the samples you
   want to test into it.
3. `sh tools/detailed_benchmarks.sh`
