#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import argparse
import sys
import platform
from cmark import CMark

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark tests.')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('--library-dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    args = parser.parse_args(sys.argv[1:])

cmark = CMark(prog=args.program, library_dir=args.library_dir)

# list of pairs consisting of input and a regex that must match the output.
pathological = {
    # note - some pythons have limit of 65535 for {num-matches} in re.
    "nested strong emph":
                (("*a **a " * 65000) + "b" + (" a** a*" * 65000),
                 re.compile("(<em>a <strong>a ){65000}b( a</strong> a</em>){65000}")),
    "nested brackets":
                 (("[" * 50000) + "a" + ("]" * 50000),
                  re.compile("\[{50000}a\]{50000}")),
    "nested block quotes":
                 ((("> " * 50000) + "a"),
                  re.compile("(<blockquote>\n){50000}")),
    "U+0000 in input":
                 ("abc\u0000de\u0000",
                  re.compile("abc\ufffd?de\ufffd?"))
    }

whitespace_re = re.compile('/s+/')
passed = 0
errored = 0
failed = 0

print("Testing pathological cases:")
for description in pathological:
    print(description)
    (inp, regex) = pathological[description]
    [rc, actual, err] = cmark.to_html(inp)
    if rc != 0:
        errored += 1
        print(description)
        print("program returned error code %d" % rc)
        print(err)
    elif regex.search(actual):
        passed += 1
    else:
        print(description, 'failed')
        print(repr(actual))
        failed += 1

print("%d passed, %d failed, %d errored" % (passed, failed, errored))
if (failed == 0 and errored == 0):
    exit(0)
else:
    exit(1)
