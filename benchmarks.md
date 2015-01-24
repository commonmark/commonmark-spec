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
