#!/usr/bin/env python

# Creates a man page from a C file.

# Comments beginning with `/**` are treated as Groff man, except that
# 'this' is converted to \fIthis\fR, and ''this'' to \fBthis\fR.

# Non-blank lines immediately following a man page comment are treated
# as function signatures or examples and parsed into .Ft, .Fo, .Fa, .Fc. The
# immediately preceding man documentation chunk is printed after the example
# as a comment on it.

# That's about it!

import sys, re, os
from datetime import date

comment_start_re = re.compile('^\/\*\* ?')
comment_delim_re = re.compile('^[/ ]\** ?')
comment_end_re = re.compile('^ \**\/')
function_re = re.compile('^ *(?:CMARK_EXPORT\s+)?(?P<type>(?:const\s+)?\w+(?:\s*[*])?)\s*(?P<name>\w+)\s*\((?P<args>[^)]*)\)')
blank_re = re.compile('^\s*$')
macro_re = re.compile('CMARK_EXPORT *')
typedef_start_re = re.compile('typedef.*{$')
typedef_end_re = re.compile('}')
single_quote_re = re.compile("(?<!\w)'([^']+)'(?!\w)")
double_quote_re = re.compile("(?<!\w)''([^']+)''(?!\w)")

def handle_quotes(s):
    return re.sub(double_quote_re, '\\\\fB\g<1>\\\\fR', re.sub(single_quote_re, '\\\\fI\g<1>\\\\fR', s))

typedef = False
mdlines = []
chunk = []
sig = []

if len(sys.argv) > 1:
    sourcefile = sys.argv[1]
else:
    print("Usage:  make_man_page.py sourcefile")
    exit(1)

with open(sourcefile, 'r') as cmarkh:
    state = 'default'
    for line in cmarkh:
        # state transition
        oldstate = state
        if comment_start_re.match(line):
            state = 'man'
        elif comment_end_re.match(line) and state == 'man':
            continue
        elif comment_delim_re.match(line) and state == 'man':
            state = 'man'
        elif not typedef and blank_re.match(line):
            state = 'default'
        elif typedef and typedef_end_re.match(line):
            typedef = False
        elif state == 'man':
            state = 'signature'
            typedef = typedef_start_re.match(line)

        # handle line
        if state == 'man':
            chunk.append(handle_quotes(re.sub(comment_delim_re, '', line)))
        elif state == 'signature':
            ln = re.sub(macro_re, '', line)
            if typedef or not re.match(blank_re, ln):
                sig.append(ln)
        elif oldstate == 'signature' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            rawsig = ''.join(sig)
            m = function_re.match(rawsig)
            if m:
                mdlines.append('\\fI' + m.group('type') + '\\fR' + ' ')
                mdlines.append('\\fB' + m.group('name') + '\\fR' + '(')
                first = True
                for argument in re.split(',', m.group('args')):
                    if not first:
                        mdlines.append(', ')
                    first = False
                    mdlines.append('\\fI' + argument.strip() + '\\fR')
                mdlines.append(')\n')
            else:
                mdlines.append('.nf\n\\f[C]\n.RS 0n\n')
                mdlines += sig
                mdlines.append('.RE\n\\f[]\n.fi\n')
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines.append('.PP\n')
            mdlines += chunk
            chunk = []
            sig = []
        elif oldstate == 'man' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += chunk # add man chunk
            chunk = []
            mdlines.append('\n')

sys.stdout.write('.TH ' + os.path.basename(sourcefile).replace('.h','') + ' 3 "' + date.today().strftime('%B %d, %Y') + '" "LOCAL" "Library Functions Manual"\n')
sys.stdout.write(''.join(mdlines))
