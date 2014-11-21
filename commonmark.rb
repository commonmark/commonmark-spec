require 'ffi'
require 'stringio'
require 'cgi'

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
  attach_function :cmark_node_get_header_level, [:node], :int
end

class Node
  attr_accessor :type, :children, :string_content, :header_level
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
    @header_level = CMark::cmark_node_get_header_level(pointer)
    if @type == :document
      self.free
    end
  end

  def self.from_markdown(s)
    len = s.bytes.length
    Node.new(CMark::cmark_parse_document(s, len))
  end

  def free
      CMark::cmark_free_nodes(@pointer)
  end
end

class Renderer
  def initialize(stream = nil)
    if stream
      @stream = stream
      @stringwriter = false
    else
      @stringwriter = true
      @stream = StringIO.new
    end
    @need_blocksep = false
  end

  def outf(format, *args)
    @stream.printf(format, *args)
  end

  def out(arg)
    @stream.write(arg)
  end

  def render(node)
    if node.kind_of?(Array)
      node.each { |x| self.render(x) }
    else
      case node.type
      when :document
        self.document(node.children)
        if @stringwriter
          return @stream.string
        end
      when :paragraph
        self.blocksep
        self.paragraph(node.children)
        @need_blocksep = true
      when :setext_header, :atx_header
        self.blocksep
        self.header(node.header_level, node.children)
        @need_blocksep = true
      when :str
        self.str(node.string_content)
      else
        # raise "unimplemented " + node.type.to_s
      end
      @last_node = node.type
    end
  end

  def blocksep
    if @need_blocksep
      self.out("\n\n")
    end
  end

  def document(children)
    self.render(children)
  end

  def header(level, children)
    self.render(children)
  end

  def paragraph(children)
    self.render(children)
  end

  def str(content)
    self.out(content)
  end
end

class HtmlRenderer < Renderer
  def header(level, children)
    self.outf("<h%d>", level)
    self.render(children)
    self.outf("</h%d>", level)
  end

  def paragraph(children)
    self.out("<p>")
    self.render(children)
    self.out("</p>")
  end

  def str(content)
    self.out(CGI.escapeHTML(content))
  end
end

doc = Node.from_markdown(STDIN.read())
renderer = HtmlRenderer.new(STDOUT)
renderer.render(doc)

# def markdown_to_html(s)
#   len = s.bytes.length
#   CMark::cmark_markdown_to_html(s, len)
# end
# print markdown_to_html(STDIN.read())
