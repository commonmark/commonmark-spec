require 'ffi'

module CMark
  extend FFI::Library
  ffi_lib ['libcmark', 'cmark']
  typedef :pointer, :node
  enum :node_type, [:document, :blockquote, :list, :list_item,
                    :fenced_code, :indented_code, :html, :paragraph,
                    :atx_header, :setext_header, :hrule, :reference_def,
                    :str, :softbreak, :linebreak, :code, :inline_html,
                    :emph, :strong, :link, :image]
  enum :node_type, []
  attach_function :cmark_markdown_to_html, [:string, :int], :string
  attach_function :cmark_parse_document, [:string, :int], :node
  attach_function :cmark_node_first_child, [:node], :node
  attach_function :cmark_node_next, [:node], :node
  attach_function :cmark_node_get_type, [:node], :node_type
end

def pointer_to_block(pointer)
  if pointer.null?
    raise NullPointer
  else
    node_type = CMark::cmark_node_get_type(pointer)
    if node_type == 0 # document
      children = []
      b = CMark::cmark_node_first_child(pointer)
      while !b.null?
        children << pointer_to_block(b)
        b = CMark::cmark_node_next(b)
      end
      return Document.new(children)
    elsif node_type == 7 # paragraph
      return Paragraph.new("TODO")
    else
      raise UnknownNodeType, node_type
    end
  end
end

class Block
  def initialize
  end
end

class Document < Block
  attr_accessor :children
  def initialize(children)
    @children = children
  end
end

class Paragraph < Block
  def initialize(inlines)
    @contents = "inlines" # TODO
  end
  def to_html
    "<p>" + @contents + "</p>"
  end
end

class Inline
end

def parse_markdown(s)
  len = s.bytes.length
  pointer_to_block(CMark::cmark_parse_document(s, len))
end

def markdown_to_html(s)
  len = s.bytes.length
  CMark::cmark_markdown_to_html(s, len)
end

doc = parse_markdown(STDIN.read())
doc.children.each do |b|
  print(b)
end
# print markdown_to_html(STDIN.read())
