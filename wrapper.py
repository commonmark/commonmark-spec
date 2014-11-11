#!/usr/bin/env python

# Example for using the shared library from python

from ctypes import CDLL, c_char_p
import sys
import platform

sysname = platform.system()

if sysname == 'Darwin':
    cmark = CDLL("build/src/libcmark.dylib")
else:
    cmark = CDLL("build/src/libcmark.so")

markdown = cmark.cmark_markdown_to_html
markdown.restype = c_char_p
markdown.argtypes = [c_char_p]

def md2html(text):
    return markdown(text, len(text))

print md2html(sys.stdin.read())
