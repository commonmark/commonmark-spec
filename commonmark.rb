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

  attach_function :cmark_free_nodes, [:node], :void
  attach_function :cmark_node_unlink, [:node], :void

  attach_function :cmark_markdown_to_html, [:string, :int], :string
  attach_function :cmark_parse_document, [:string, :int], :node
  attach_function :cmark_node_first_child, [:node], :node
  attach_function :cmark_node_parent, [:node], :node
  attach_function :cmark_node_next, [:node], :node
  attach_function :cmark_node_previous, [:node], :node
  attach_function :cmark_node_get_type, [:node], :node_type
  attach_function :cmark_node_get_string_content, [:node], :string

  class Node
    attr_accessor :type, :children, :string_content
    def initialize(pointer)
      if pointer.null?
        return nil
      end
      @pointer = pointer
      @type = CMark::cmark_node_get_type(pointer)
      @children = []
      first_child = CMark::cmark_node_first_child(pointer)
      b = first_child
      while !b.null?
        @children << Node.new(b)
        b = CMark::cmark_node_next(b)
      end
      @string_content = CMark::cmark_node_get_string_content(pointer)
      # Free?
    end

    def print
      printf("%s\n", self.type)
      if self.string_content
        printf("'%s'\n", self.string_content)
      end
      for child in self.children do
        child.print
      end
    end

    def self.from_markdown(s)
      len = s.bytes.length
      Node.new(CMark::cmark_parse_document(s, len))
    end
  end

end

doc = CMark::Node.from_markdown(STDIN.read())
doc.print

# def markdown_to_html(s)
#   len = s.bytes.length
#   CMark::cmark_markdown_to_html(s, len)
# end
# print markdown_to_html(STDIN.read())
