#!/usr/bin/env ruby
require 'ffi'

module CMark
  extend FFI::Library
  ffi_lib ['libcmark', 'cmark']
  attach_function :cmark_markdown_to_html, [:string, :int], :string
end

def markdown_to_html(s)
  len = s.bytesize
  CMark::cmark_markdown_to_html(s, len)
end

STDOUT.write(markdown_to_html(ARGF.read()))
