#!/usr/bin/env python3
import re
import sys

stage = 0
example = 0
section = ""

if len(sys.argv) > 1:
    specfile = sys.argv[1]
else:
    specfile = 'spec.txt'

with open(specfile, 'r', encoding='utf-8') as spec:
    for ln in spec:
        if re.match(r'^\.$', ln):
            if stage == 0:
                example += 1
                sys.stdout.write("\n<div class=\"example\" id=\"example-{0}\" data-section=\"{1}\">\n".format(example, section))
                sys.stdout.write("<div class=\"examplenum\"><a href=\"#example-{0}\">Example {0}</a>&nbsp;&nbsp;<a class=\"dingus\" title=\"open in interactive dingus\">(interact)</a></div>\n\n".format(example))
                sys.stdout.write("````````````````````````````````````````````````````````` markdown\n")
                stage = 1
            elif stage == 1:
                sys.stdout.write("`````````````````````````````````````````````````````````\n\n")
                sys.stdout.write("````````````````````````````````````````````````````````` html\n")
                stage = 2
            elif stage == 2:
                sys.stdout.write("`````````````````````````````````````````````````````````\n\n")
                sys.stdout.write("</div>\n")
                stage = 0
            else:
                sys.stderr.out("Encountered unknown stage {0}\n".format(stage))
                sys.exit(1)
        else:
            if stage == 0:
                match = re.match(r'^#{1,6} *(.*)', ln)
                if match:
                    section = match.group(1)
            else:
                ln = re.sub(r' ', '␣', ln)
            sys.stdout.write(ln)
