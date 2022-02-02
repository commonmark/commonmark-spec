#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
The CMark library (https://github.com/commonmark/cmark) uses the same API so
changes made here should be made backwards-compatible to facilitate porting.
"""

from ctypes import CDLL, c_char_p, c_long
from subprocess import *
import platform
import os

def pipe_through_prog(prog, text, decode):
    p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
    [result, err] = p1.communicate(input=text.encode('utf-8'))
    if decode:
        result = result.decode('utf-8')
    return [p1.returncode, result, err]

def use_library(lib, text, decode):
    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    result = lib(textbytes, textlen, 0)
    if decode:
        result = result.decode('utf-8')
    return [0, result, '']

class CMark:
    def __init__(self, prog=None, library_dir=None, decode_html=True):
        self.prog = prog
        if prog:
            self.to_html = lambda x: pipe_through_prog(prog, x, decode_html)
        else:
            sysname = platform.system()
            if sysname == 'Darwin':
                libname = "libcmark.dylib"
            elif sysname == 'Windows':
                libname = "cmark.dll"
            else:
                libname = "libcmark.so"
            if library_dir:
                libpath = os.path.join(library_dir, libname)
            else:
                libpath = os.path.join("build", "src", libname)
            cmark = CDLL(libpath)
            markdown = cmark.cmark_markdown_to_html
            markdown.restype = c_char_p
            markdown.argtypes = [c_char_p, c_long]
            self.to_html = lambda x: use_library(markdown, x, decode_html)
