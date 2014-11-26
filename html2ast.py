#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from HTMLParser import HTMLParser, HTMLParseError
import re
from htmlentitydefs import name2codepoint
import cgi

class MyHTMLParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.in_pre = False
        self.output = u""
    def handle_data(self, data):
        self.output += data
    def handle_endtag(self, tag):
        if tag == "pre":
            self.in_pre = False
    def handle_starttag(self, tag, attrs):
        if tag == "pre":
            self.in_pre = True
    def handle_startendtag(self, tag, attrs):
        self.output += ""
    def handle_comment(self, data):
        self.output += '<!--' + data + '-->'
    def handle_decl(self, data):
        self.output += '<!' + data + '>'
    def unknown_decl(self, data):
        self.output += '<!' + data + '>'
    def handle_pi(self,data):
        self.output += '<?' + data + '>'
    def handle_entityref(self, name):
        try:
            c = unichr(name2codepoint[name])
        except KeyError:
            c = None
        self.output_char(c)
    def handle_charref(self, name):
        try:
            if name.startswith("x"):
                c = unichr(int(name[1:], 16))
            else:
                c = unichr(int(name))
        except ValueError:
                c = None
        self.output_char(c)
    # Helpers.
    def output_char(self, c):
        if c == u'\n':
            self.output += "\\n"
        elif c == u'"':
            self.output += "\\\""
        elif c == u'\\':
            self.output += "\\\\"
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

def html2ast(html):
    html_chunk_re = re.compile("(\<!\[CDATA\[.*?\]\]\>|\<[^>]*\>|[^<]+)")
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


if __name__ == "__main__":
    print(html2ast(sys.stdin.read()))
