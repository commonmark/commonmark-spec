#!/usr/bin/env python
# -*- coding: utf-8 -*-

from ctypes import CDLL, c_char_p, c_long
import sys
import platform
from difflib import unified_diff
from subprocess import *
import argparse
from HTMLParser import HTMLParser
from htmlentitydefs import name2codepoint
import re

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark tests.')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('--spec', dest='spec', nargs='?', default='spec.txt',
            help='path to spec')
    parser.add_argument('--pattern', dest='pattern', nargs='?',
            default=None, help='limit to sections matching regex pattern')
    parser.add_argument('--library_dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    args = parser.parse_args(sys.argv[1:])

if not args.program:
    sysname = platform.system()
    libname = "libcmark"
    if sysname == 'Darwin':
        libname += ".dylib"
    elif sysname == 'Windows':
        libname += ".dll"
    else:
        libname += ".so"
    if args and args.library_dir:
        libpath = args.library_dir + "/" + libname
    else:
        libpath = "build/src/" + libname
    cmark = CDLL(libpath)

    markdown = cmark.cmark_markdown_to_html
    markdown.restype = c_char_p
    markdown.argtypes = [c_char_p, c_long]

def md2html(text, prog):
    if prog:
        p1 = Popen([prog], stdout=PIPE, stdin=PIPE, stderr=PIPE)
        [result, err] = p1.communicate(input=text)
        return [p1.returncode, result, err]
    else:
        return [0, markdown(text, len(text)), '']

# Normalization code, adapted from
# https://github.com/karlcow/markdown-testsuite/
significant_attrs = ["alt", "href", "src", "title"]
normalize_whitespace_re = re.compile('\s+')
class MyHTMLParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.last = "starttag"
        self.in_pre = False
        self.output = u""
    def handle_data(self, data):
        if self.in_pre:
            self.output += data
        else:
            data = normalize_whitespace_re.sub(' ', data)
            data_strip = data.strip()
            if (self.last == "ref") and data_strip and data[0] == " ":
                self.output += " "
            self.data_end_in_space_not_empty = (data[-1] == ' ' and data_strip)
            self.output += data_strip
            self.last = "data"
    def handle_endtag(self, tag):
        if tag == "pre":
            self.in_pre = False
        self.output += "</" + tag + ">"
        self.last = "endtag"
    def handle_starttag(self, tag, attrs):
        if tag == "pre":
            self.in_pre = True
        self.output += "<" + tag
        attrs = filter(lambda attr: attr[0] in significant_attrs, attrs)
        if attrs:
            attrs.sort()
            for attr in attrs:
                self.output += " " + attr[0] + "=" + '"' + attr[1] + '"'
        self.output += ">"
        self.last = "starttag"
    def handle_startendtag(self, tag, attrs):
        """Ignore closing tag for self-closing void elements."""
        self.handle_starttag(tag, attrs)
    def handle_entityref(self, name):
        self.add_space_from_last_data()
        try:
            self.output += unichr(name2codepoint[name])
        except KeyError:
            self.output += name
        self.last = "ref"
    def handle_charref(self, name):
        self.add_space_from_last_data()
        try:
            if name.startswith("x"):
                c = unichr(int(name[1:], 16))
            else:
                c = unichr(int(name))
            self.output += c
        except ValueError:
            self.output += name
        self.last = "ref"
    # Helpers.
    def add_space_from_last_data(self):
        """Maintain the space at: `a <span>b</span>`"""
        if self.last == 'data' and self.data_end_in_space_not_empty:
            self.output += ' '

def normalize(html):
    r"""
    Return normalized form of HTML which igores insignificant output differences.
    Multiple inner whitespaces to a single space
        >>> normalize("<p>a  \t\nb</p>")
        u'<p>a b</p>'
    Surrounding whitespaces are removed:
        >>> normalize("<p> a</p>")
        u'<p>a</p>'
        >>> normalize("<p>a </p>")
        u'<p>a</p>'
    TODO: how to deal with the following cases without a full list of the void tags?
        >>> normalize("<p>a <b>b</b></p>")
        u'<p>a<b>b</b></p>'
        >>> normalize("<p><b>b</b> c</p>")
        u'<p><b>b</b>c</p>'
        >>> normalize("<p>a <br></p>")
        u'<p>a<br></p>'
    `pre` elements preserve whitespace:
        >>> normalize("<pre>a  \t\nb</pre>")
        u'<pre>a  \t\nb</pre>'
    Self-closing tags:
        >>> normalize("<p><br /></p>")
        u'<p><br></p>'
    References are converted to Unicode:
        >>> normalize("<p>&lt;</p>")
        u'<p><</p>'
        >>> normalize("<p>&#60;</p>")
        u'<p><</p>'
        >>> normalize("<p>&#x3C;</p>")
        u'<p><</p>'
        >>> normalize("<p>&#x4E2D;</p>")
        u'<p>\u4e2d</p>'
    Spaces around entities are kept:
        >>> normalize("<p>a &lt; b</p>")
        u'<p>a < b</p>'
        >>> normalize("<p>a&lt;b</p>")
        u'<p>a<b</p>'
    Most attributes are ignored:
        >>> normalize('<p id="a"></p>')
        u'<p></p>'
    Critical attributes are considered and sorted alphabetically:
        >>> normalize('<a href="a"></a>')
        u'<a href="a"></a>'
        >>> normalize('<img src="a" alt="a">')
        u'<img alt="a" src="a">'
    """
    parser = MyHTMLParser()
    parser.feed(html.decode(encoding='UTF-8'))
    parser.close()
    return parser.output

def print_test_header(headertext, example_number, start_line, end_line):
    print "Example %d (lines %d-%d) %s" % (example_number,start_line,end_line,headertext)

def do_test(markdown_lines, expected_html_lines, headertext,
            example_number, start_line, end_line, prog=None):
    real_markdown_text = ''.join(markdown_lines).replace('→','\t')
    [retcode, actual_html, err] = md2html(real_markdown_text, prog)
    if retcode == 0:
        actual_html_lines = actual_html.splitlines(True)
        expected_html = ''.join(expected_html_lines)
        if normalize(actual_html) == normalize(expected_html):
            return 'pass'
        else:
            print_test_header(headertext, example_number,start_line,end_line)
            sys.stdout.write(real_markdown_text)
            for diffline in unified_diff(expected_html_lines, actual_html_lines,
                            "expected HTML", "actual HTML"):
                sys.stdout.write(diffline)
            sys.stdout.write('\n')
            return 'fail'
    else:
        print_test_header(example_number,start_line,end_line)
        print "program returned error code %d" % retcode
        print(err)
        return 'error'

def do_tests(specfile, prog, pattern):
    line_number = 0
    start_line = 0
    end_line = 0
    example_number = 0
    passed = 0
    failed = 0
    errored = 0
    markdown_lines = []
    html_lines = []
    active = True
    state = 0  # 0 regular text, 1 markdown example, 2 html output
    headertext = ''

    header_re = re.compile('#+ ')
    if pattern:
        pattern_re = re.compile(pattern)

    with open(specfile, 'r') as specf:
        for line in specf:
            line_number = line_number + 1
            if state == 0 and re.match(header_re, line):
                headertext = header_re.sub('', line).strip()
                if pattern:
                    if re.search(pattern_re, line):
                        active = True
                    else:
                        active = False
            if line.strip() == ".":
                state = (state + 1) % 3
                if state == 0:
                    example_number = example_number + 1
                    end_line = line_number
                    if active:
                        result = do_test(markdown_lines, html_lines,
                                         headertext, example_number,
                                         start_line, end_line, prog)
                        if result == 'pass':
                            passed = passed + 1
                        elif result == 'fail':
                            failed = failed + 1
                        else:
                            errored = errored + 1
                    start_line = 0
                    markdown_lines = []
                    html_lines = []
            elif state == 1:
                if start_line == 0:
                    start_line = line_number
                markdown_lines.append(line)
            elif state == 2:
                html_lines.append(line)
    print "%d passed, %d failed, %d errored" % (passed, failed, errored)
    return (failed == 0 and errored == 0)

if __name__ == "__main__":
    if do_tests(args.spec, args.program, args.pattern):
        exit(0)
    else:
        exit(1)
