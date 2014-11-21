require 'ffi'
require 'stringio'

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
    attr_accessor :type, :children, :string_content, :pointer
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
      if @type == :document
        self.free
      end
    end

    def to_html_stream(file)
      file.printf("[%s]\n", self.type.to_s)
      case self.type
      when :document
        self.children.each do |child|
          child.to_html_stream(file)
        end
      when :paragraph
        self.children.each do |child|
          child.to_html_stream(file)
        end
      when :str
        file.puts(self.string_content)
      else
      end
    end

    def to_html
      self.to_html_stream(StringIO.new)
    end

    def self.from_markdown(s)
      len = s.bytes.length
      Node.new(CMark::cmark_parse_document(s, len))
    end

    def free
        CMark::cmark_free_nodes(@pointer)
    end
  end

end

doc = CMark::Node.from_markdown(STDIN.read())
doc.to_html_stream(STDOUT)

# def markdown_to_html(s)
#   len = s.bytes.length
#   CMark::cmark_markdown_to_html(s, len)
# end
# print markdown_to_html(STDIN.read())
