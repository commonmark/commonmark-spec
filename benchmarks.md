# Benchmarks

Some benchmarks, run on an ancient Thinkpad running Intel Core 2 Duo at 2GHz.

|Implementation |  Time (sec)| Factor  |
|---------------|-----------:|--------:|
| Markdown.pl   | 2921.30    | 14606.5 |
| kramdown      | 20.76      |   103.8 |
| PHP markdown  | 20.475     |   102.4 |
| lunamark      | 6.195      |    40.0 |
| cheapskate    | 5.645      |    28.2 |
| peg-markdown  | 5.330      |    26.7 |
| commonmark.js | 2.730      |    13.6 |
| marked        | 1.870      |     9.4 |
| remarkable.js | 1.790      |     8.9 |
| discount      | 1.660      |     8.3 |
| **cmark**     | 0.320      |     1.6 |
| sundown       | 0.200      |     1.0 |

To run these benchmarks, use `make bench PROG=/path/to/program`.

The input text is a 10MB Markdown file built by concatenating 20 copies
of the Markdown source of the first edition of [*Pro
Git*](https://github.com/progit/progit/tree/master/en) by Scott Chacon.

`time` is used to measure execution speed.

