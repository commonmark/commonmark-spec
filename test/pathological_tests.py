#!/usr/bin/env python
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

pathological = {
    "nested strong emph":
                (("*a **a " * 100000) + "b" + (" a** a*" * 100000),
                 "<p>" + ("<em>a <strong>a " * 100000) + "b" +
                   (" a</strong> a</em>" * 100000) + "</p>"),
    "nested brackets":
                 (("[" * 50000) + "a" + ("]" * 50000),
                  "<p>" + ("[" * 50000) + "a" + ("]" * 50000) + "</p>")
    }

whitespace_re = re.compile('/s+/')
passed = 0
errored = 0
failed = 0

print "Testing pathological cases:"
for description in pathological:
    print description
    (inp, expected) = pathological[description]
    [rc, actual, err] = cmark.to_html(inp)
    if rc != 0:
        errored += 1
        print description
        print "program returned error code %d" % rc
        print(err)
    elif whitespace_re.sub(' ', actual.rstrip()) == expected.rstrip():
        passed += 1
    else:
        print description, 'failed'
        print(actual)
        failed += 1

print "%d passed, %d failed, %d errored" % (passed, failed, errored)
if (failed == 0 and errored == 0):
    exit(0)
else:
    exit(1)
