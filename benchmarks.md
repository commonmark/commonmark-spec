# Benchmarks

Some benchmarks, run on an ancient Thinkpad running Intel Core 2 Duo at 2GHz.

|Implementation     |  Time (sec)| Factor  |
|-------------------|-----------:|--------:|
| Markdown.pl       | 2921.24    | 14606.2 |
| PHP markdown      | 20.85      |   104.3 |
| kramdown          | 20.83      |   104.1 |
| lunamark          | 6.295      |    31.5 |
| cheapskate        | 5.760      |    28.8 |
| peg-markdown      | 5.450      |    27.3 |
| **commonmark.js** | 2.675      |    13.4 |
| marked            | 1.855      |     9.3 |
| discount          | 1.705      |     8.5 |
| **cmark**         | 0.295      |     1.5 |
| sundown           | 0.200      |     1.0 |


To run these benchmarks, use `make bench PROG=/path/to/program`.

The input text is a 10MB Markdown file built by concatenating 20 copies
of the Markdown source of the first edition of [*Pro
Git*](https://github.com/progit/progit/tree/master/en) by Scott Chacon.

`time` is used to measure execution speed.  The reported
time is the *difference* between the time to run the program
with the benchmark input and the time to run it with no input.
(This procedure ensures that implementations in dynamic languages are
not prenalized by startup time.) A median of ten runs is taken.  The
process is reniced to a high priority so that the system doesn't
interrupt runs.

