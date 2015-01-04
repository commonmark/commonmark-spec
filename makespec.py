#!/usr/bin/env python3
import re
import sys
from subprocess import *

if len(sys.argv) == 3:
    specfile = sys.argv[1]
    specformat = sys.argv[2]
    if not (specformat in ["html", "markdown"]):
        sys.stderr.write("Format must be html or markdown\n")
        exit(1)
else:
    sys.stderr.write("Usage:  makespec.py SPECFILE [html|markdown]\n")
    exit(1)

def pipe_through_prog(prog, text):
    p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
    [result, err] = p1.communicate(input=text.encode('utf-8'))
    return [p1.returncode, result.decode('utf-8'), err]

def replaceAnchor(match):
    refs.append("[{0}]: #{1}".format(match.group(1), match.group(2)))
    if specformat == "html":
        return '<a id="{1}" href="#{1}" class="definition">{0}</a>'.format(match.group(1), match.group(2))
    else:
        return match.group(0)

stage = 0
example = 0
section = ""
mdlines = []
refs = []

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
                    ln = re.sub(r'\[([^]]*)\]\(@([^)]*)\)', replaceAnchor, ln)
            else:
                ln = re.sub(r' ', '␣', ln)
            mdlines.append(ln)

mdtext = ''.join(mdlines) + '\n\n' + '\n'.join(refs) + '\n'

if specformat == "markdown":
    sys.stdout.write(mdtext)
elif specformat == "html":
    prog = "pandoc -s --toc -S --no-highlight --number-sections --template template.html"
    [retcode, result, err] = pipe_through_prog(prog, mdtext)
    if retcode == 0:
        result = re.sub(r'␣', '<span class="space"> </span>', result)
        sys.stdout.write(result)
    else:
        sys.stderr.write("Error converting markdown version of spec:\n")
        sys.stderr.write(err)
        exit(1)

exit(0)
