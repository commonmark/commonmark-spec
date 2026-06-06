#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ctypes import *
from subprocess import Popen, PIPE
import platform
import os

class cmark_mem(Structure):
    _fields_ = [("calloc", c_void_p),
                ("realloc", c_void_p),
                ("free", CFUNCTYPE(None, c_void_p))]

def pipe_through_prog(prog, text):
    p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
    [result, err] = p1.communicate(input=text.encode('utf-8'))
    return [p1.returncode, result.decode('utf-8'), err]

def to_html(lib, text):
    get_alloc = lib.cmark_get_default_mem_allocator
    get_alloc.restype = POINTER(cmark_mem)
    free_func = get_alloc().contents.free

    markdown = lib.cmark_markdown_to_html
    markdown.restype = POINTER(c_char)
    markdown.argtypes = [c_char_p, c_size_t, c_int]

    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    # 1 << 17 == CMARK_OPT_UNSAFE
    cstring = markdown(textbytes, textlen, 1 << 17)
    result = string_at(cstring).decode('utf-8')
    free_func(cstring)

    return [0, result, '']

def to_commonmark(lib, text):
    get_alloc = lib.cmark_get_default_mem_allocator
    get_alloc.restype = POINTER(cmark_mem)
    free_func = get_alloc().contents.free

    parse_document = lib.cmark_parse_document
    parse_document.restype = c_void_p
    parse_document.argtypes = [c_char_p, c_size_t, c_int]

    render_commonmark = lib.cmark_render_commonmark
    render_commonmark.restype = POINTER(c_char)
    render_commonmark.argtypes = [c_void_p, c_int, c_int]

    free_node = lib.cmark_node_free
    free_node.argtypes = [c_void_p]

    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    node = parse_document(textbytes, textlen, 0)
    cstring = render_commonmark(node, 0, 0)
    result = string_at(cstring).decode('utf-8')
    free_func(cstring)
    free_node(node)

    return [0, result, '']

class CMark:
    def __init__(self, prog=None, library_dir=None):
        self.prog = prog
        if prog:
            prog += ' --unsafe'
            self.to_html = lambda x: pipe_through_prog(prog, x)
            self.to_commonmark = lambda x: pipe_through_prog(prog + ' -t commonmark', x)
        else:
            sysname = platform.system()
            if sysname == 'Darwin':
                libnames = [ "libcmark.dylib" ]
            elif sysname == 'Windows':
                libnames = [ "cmark.dll", "libcmark.dll" ]
            else:
                libnames = [ "libcmark.so" ]
            if not library_dir:
                library_dir = os.path.join("build", "src")
            for libname in libnames:
                candidate = os.path.join(library_dir, libname)
                if os.path.isfile(candidate):
                    libpath = candidate
                    break
            cmark = CDLL(libpath)
            self.to_html = lambda x: to_html(cmark, x)
            self.to_commonmark = lambda x: to_commonmark(cmark, x)
