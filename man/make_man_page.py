#!/usr/bin/env python

# Creates a man page from a C file.

# Lines beginning with /// are treated as Markdown.

# Non-blank lines immediately following a /// line are treated
# as function signatures or examples and included verbatim. The
# immediately preceding markdown chunk is printed after the example
# as a comment on it.

# That's about it!

import sys
import re

if len(sys.argv) > 1:
    sourcefile = sys.argv[1]
else:
    print("Usage:  make_man_page.py sourcefile")
    exit(1)

special_comment_re = re.compile('^\/\/\/ ?')
blank_re = re.compile('^\s*$')
macro_re = re.compile('CMARK_EXPORT *')

mdlines = []
chunk = []
sig = []

with open(sourcefile, 'r') as cmarkh:
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
        #if oldstate != state and len(mdlines) > 0 and mdlines[-1] != '\n':
        #    mdlines.append('\n')
        if state == 'markdown':
            chunk.append(re.sub(special_comment_re, '', line))
        elif state == 'signature':
            sig.append('    ' + re.sub(macro_re, '', line))
        elif oldstate == 'signature' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += sig   # add sig, then prepended markdown comment
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += chunk
            chunk = []
            sig = []
        elif oldstate == 'markdown' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += chunk # add markdown chunk
            chunk = []
            mdlines.append('\n')

sys.stdout.write(''.join(mdlines))
