require 'ffi'

module CMark
  extend FFI::Library
  ffi_lib ['libcmark', 'cmark']
  attach_function :cmark_markdown_to_html, [:string, :int], :string
end

def markdown_to_html(s)
  CMark::cmark_markdown_to_html(s, s.length)
end

print markdown_to_html(STDIN.read());

