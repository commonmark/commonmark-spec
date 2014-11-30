#!/usr/bin/env python

# Creates a man page from a C file.
# Lines beginning with /// are treated as Markdown.
# Non-blank lines immediately following a /// line are treated
# as function signatures or examples and included verbatim.
# That's about it!

import sys
import re

special_comment_re = re.compile('\/\/\/');
blank_re = re.compile('^\s*$');

mdlines = []

with open('../src/cmark.h', 'r') as cmarkh:
    state = 'default'
    for line in cmarkh:
        # state transition
        oldstate = state
        if special_comment_re.match(line):
            state = 'markdown'
        elif blank_re.match(line):
            state = 'default'
        elif state == 'markdown':
            state = 'signature'

        # handle line
        if oldstate != state and len(mdlines) > 0 and mdlines[-1] != '\n':
            mdlines.append('\n')
        if state == 'markdown':
            mdlines.append(line[4:])
        elif state == 'signature':
            mdlines.append('    ' + line)

sys.stdout.write(''.join(mdlines))
