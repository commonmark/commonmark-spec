#!/usr/bin/env python
# -*- coding: utf-8 -*-

from ctypes import CDLL, c_char_p, c_long
import sys
import platform
from difflib import unified_diff
from subprocess import *
import argparse
from HTMLParser import HTMLParser, HTMLParseError
from htmlentitydefs import name2codepoint
import re
import cgi
import json

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark tests.')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('--spec', dest='spec', nargs='?', default='spec.txt',
            help='path to spec')
    parser.add_argument('--pattern', dest='pattern', nargs='?',
            default=None, help='limit to sections matching regex pattern')
    parser.add_argument('--library-dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    parser.add_argument('--no-normalize', dest='normalize',
            action='store_const', const=False, default=True,
            help='do not normalize HTML')
    parser.add_argument('--dump-tests', dest='dump_tests',
            action='store_const', const=True, default=False,
            help='dump tests in JSON format')
    parser.add_argument('--debug-normalization', dest='debug_normalization',
            action='store_const', const=True,
            default=False, help='filter stdin through normalizer for testing')
    args = parser.parse_args(sys.argv[1:])

if not (args.program or args.dump_tests or args.debug_normalization):
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
        p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
        [result, err] = p1.communicate(input=text)
        return [p1.returncode, result, err]
    else:
        return [0, markdown(text, len(text)), '']

# Normalization code, adapted from
# https://github.com/karlcow/markdown-testsuite/
significant_attrs = ["alt", "href", "src", "title"]
whitespace_re = re.compile('/s+/')
class MyHTMLParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.last = "starttag"
        self.in_pre = False
        self.output = u""
        self.last_tag = ""
    def handle_data(self, data):
        after_tag = self.last == "endtag" or self.last == "starttag"
        after_block_tag = after_tag and self.is_block_tag(self.last_tag)
        if after_tag and self.last_tag == "br":
            data = data.lstrip('\n')
        data = whitespace_re.sub(' ', data)
        if after_block_tag and not self.in_pre:
            if self.last == "starttag":
                data = data.lstrip()
            elif self.last == "endtag":
                data = data.strip()
        self.output += data
        self.last = "data"
    def handle_endtag(self, tag):
        if tag == "pre":
            self.in_pre = False
        if self.is_block_tag(tag):
            self.output = self.output.rstrip()
        self.output += "</" + tag + ">"
        self.last_tag = tag
        self.last = "endtag"
    def handle_starttag(self, tag, attrs):
        if tag == "pre":
            self.in_pre = True
        self.output += "<" + tag
        # For now we don't strip out 'extra' attributes, because of
        # raw HTML test cases.
        # attrs = filter(lambda attr: attr[0] in significant_attrs, attrs)
        if attrs:
            attrs.sort()
            for (k,v) in attrs:
                self.output += " " + k
                if v != None:
                    self.output += ("=" + '"' + cgi.escape(v,quote=True) + '"')
        self.output += ">"
        self.last_tag = tag
        self.last = "starttag"
    def handle_startendtag(self, tag, attrs):
        """Ignore closing tag for self-closing """
        self.handle_starttag(tag, attrs)
        self.last_tag = tag
        self.last = "endtag"
    def handle_comment(self, data):
        self.output += '<!--' + data + '-->'
        self.last = "comment"
    def handle_decl(self, data):
        self.output += '<!' + data + '>'
        self.last = "decl"
    def unknown_decl(self, data):
        self.output += '<!' + data + '>'
        self.last = "decl"
    def handle_pi(self,data):
        self.output += '<?' + data + '>'
        self.last = "pi"
    def handle_entityref(self, name):
        try:
            c = unichr(name2codepoint[name])
        except KeyError:
            c = None
        self.output_char(c, '&' + name + ';')
        self.last = "ref"
    def handle_charref(self, name):
        try:
            if name.startswith("x"):
                c = unichr(int(name[1:], 16))
            else:
                c = unichr(int(name))
        except ValueError:
                c = None
        self.output_char(c, '&' + name + ';')
        self.last = "ref"
    # Helpers.
    def output_char(self, c, fallback):
        if c == u'<':
            self.output += "&lt;"
        elif c == u'>':
            self.output += "&gt;"
        elif c == u'&':
            self.output += "&amp;"
        elif c == u'"':
            self.output += "&quot;"
        elif c == None:
            self.output += fallback
        else:
            self.output += c

    def is_block_tag(self,tag):
        return (tag in ['article', 'header', 'aside', 'hgroup', 'blockquote',
            'hr', 'iframe', 'body', 'li', 'map', 'button', 'object', 'canvas',
            'ol', 'caption', 'output', 'col', 'p', 'colgroup', 'pre', 'dd',
            'progress', 'div', 'section', 'dl', 'table', 'td', 'dt',
            'tbody', 'embed', 'textarea', 'fieldset', 'tfoot', 'figcaption',
            'th', 'figure', 'thead', 'footer', 'tr', 'form', 'ul',
            'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'video', 'script', 'style'])

def normalize_html(html):
    r"""
    Return normalized form of HTML which ignores insignificant output
    differences:

    * Multiple inner whitespaces are collapsed to a single space (except
      in pre tags).
    * Outer whitespace (outside block-level tags) is removed.
    * Self-closing tags are converted to open tags.
    * Attributes are sorted and lowercased.
    * References are converted to unicode, except that '<', '>', '&', and
      '&' are rendered using entities.
    """
    html_chunk_re = re.compile("(\<!\[CDATA\[.*?\]\]\>|\<[^>]*\>|[^<]+)")
    try:
        parser = MyHTMLParser()
        # We work around HTMLParser's limitations parsing CDATA
        # by breaking the input into chunks and passing CDATA chunks
        # through verbatim.
        for chunk in re.finditer(html_chunk_re, html):
            if chunk.group(0)[:8] == "<![CDATA":
                parser.output += chunk.group(0)
            else:
                parser.feed(chunk.group(0).decode(encoding='UTF-8'))
        parser.close()
        return parser.output
    except HTMLParseError as e:
        sys.stderr.write("Normalization error: " + e.msg + "\n")
        return html  # on error, return unnormalized HTML

def print_test_header(headertext, example_number, start_line, end_line):
    print "Example %d (lines %d-%d) %s" % (example_number,start_line,end_line,headertext)

def do_test(markdown_lines, expected_html_lines, headertext,
            example_number, start_line, end_line, prog, normalize):
    real_markdown_text = ''.join(markdown_lines).replace('→','\t')
    [retcode, actual_html, err] = md2html(real_markdown_text, prog)
    if retcode == 0:
        actual_html_lines = actual_html.splitlines(True)
        expected_html = ''.join(expected_html_lines)
        if normalize:
            passed = normalize_html(actual_html) == normalize_html(expected_html)
        else:
            passed = actual_html == expected_html
        if passed:
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
        print_test_header(headertext, example_number, start_line, end_line)
        print "program returned error code %d" % retcode
        print(err)
        return 'error'

def do_tests(specfile, prog, pattern, normalize, dump_tests):
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
    tests_json = []

    header_re = re.compile('#+ ')
    if pattern:
        pattern_re = re.compile(pattern, re.IGNORECASE)

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
                        if dump_tests:
                            tests_json.append({
                                 "markdown":''.join(markdown_lines),
                                 "html":''.join(html_lines),
                                 "example": example_number,
                                 "start_line": start_line,
                                 "end_line": end_line,
                                 "section": headertext})
                        else:
                            result = do_test(markdown_lines, html_lines,
                                             headertext, example_number,
                                             start_line, end_line, prog,
                                             normalize)
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
                    start_line = line_number - 1
                markdown_lines.append(line)
            elif state == 2:
                html_lines.append(line)

    if dump_tests:
        print json.dumps(tests_json, ensure_ascii=False, indent=2)
    else:
        print "%d passed, %d failed, %d errored" % (passed, failed, errored)
        return (failed == 0 and errored == 0)

if __name__ == "__main__":
    if args.debug_normalization:
        print normalize_html(sys.stdin.read())
    elif do_tests(args.spec, args.program, args.pattern, args.normalize,
                  args.dump_tests):
        exit(0)
    else:
        exit(1)
