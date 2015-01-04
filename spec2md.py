#!/usr/bin/env python3
import re
import sys

stage = 0
example = 0
section = ""
mdlines = []
refs = []

if len(sys.argv) > 1:
    specfile = sys.argv[1]
else:
    specfile = 'spec.txt'

with open(specfile, 'r', encoding='utf-8') as spec:
    for ln in spec:
        if re.match(r'^\.$', ln):
            if stage == 0:
                example += 1
                mdlines.append("\n<div class=\"example\" id=\"example-{0}\" data-section=\"{1}\">\n".format(example, section))
                mdlines.append("<div class=\"examplenum\"><a href=\"#example-{0}\">Example {0}</a>&nbsp;&nbsp;<a class=\"dingus\" title=\"open in interactive dingus\">(interact)</a></div>\n\n".format(example))
                mdlines.append("````````````````````````````````````````````````````````` markdown\n")
                stage = 1
            elif stage == 1:
                mdlines.append("`````````````````````````````````````````````````````````\n\n")
                mdlines.append("````````````````````````````````````````````````````````` html\n")
                stage = 2
            elif stage == 2:
                mdlines.append("`````````````````````````````````````````````````````````\n\n")
                mdlines.append("</div>\n")
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
                    for match in re.finditer(r'\[([^]]*)\]\(@([^)]*)\)', ln):
                        refs.append("[{0}]: #{1}".format(match.group(1), match.group(2)))
            else:
                ln = re.sub(r' ', '␣', ln)
            mdlines.append(ln)

mdtext = ''.join(mdlines) + '\n\n' + '\n'.join(refs) + '\n'

sys.stdout.write(mdtext)
