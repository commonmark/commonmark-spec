# Benchmarks

Some benchmarks, run on an ancient Thinkpad running Intel Core 2 Duo at 2GHz.

|Implementation |  Time (sec)| Factor|
|---------------|------------|-------|
| Markdown.pl   | 2921.30    | 14606 |
| PHP markdown  | 20.475     | 102.4 |
| peg-markdown  | 5.330      |  26.7 |
| commonmark.js | 2.730      |  13.6 |
| marked        | 1.870      |   9.4 |
| discount      | 1.660      |   8.3 |
| cmark         | 0.290      |   1.4 |
| sundown       | 0.200      |   1.0 |

To run these benchmarks, use `make bench PROG=/path/to/program`.

The input text is a 10MB Markdown file built by concatenating 20 copies
of the Markdown source of the first edition of [*Pro
Git*](https://github.com/progit/progit/tree/master/en) by Scott Chacon.

`time` is used to measure execution speed.

